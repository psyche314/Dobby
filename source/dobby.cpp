#include "dobby.h"
#include "dobby/common.h"
#include "Interceptor.h"
#include "InterceptRouting/InlineHookRouting.h"
#include "InterceptRouting/InstrumentRouting.h"
#include "MemoryAllocator/NearMemoryAllocator.h"
#include <mutex>

namespace {
std::mutex interceptor_mutex;

addr_t entry_address(void *address) {
  features::apple::arm64e_pac_strip(address);
  features::arm_thumb_fix_addr(address);
  return (addr_t)address;
}

void delete_entry(Interceptor::Entry *entry) {
  delete entry->routing;
  delete entry;
}

int prepare(void *address, void *replacement, void **original) {
  if (!address || !replacement)
    return -1;
  auto key = entry_address(address);
  if (key == entry_address(replacement) || gInterceptor.find(key))
    return -1;
  features::apple::arm64e_pac_strip(address);
  features::apple::arm64e_pac_strip(replacement);
  auto entry = new Interceptor::Entry((addr_t)address);
  entry->fake_func_addr = (addr_t)replacement;
  auto routing = new InlineHookRouting(entry, (addr_t)replacement);
  entry->routing = routing;
  routing->BuildRouting();
  if (routing->error) {
    delete_entry(entry);
    return -1;
  }
  gInterceptor.add(entry);
  if (original) {
    *original = (void *)entry->relocated.addr();
    features::apple::arm64e_pac_strip_and_sign(*original);
  }
  return 0;
}

int set_enabled(Interceptor::Entry *entry, bool enabled) {
  if (!entry)
    return -1;
  auto address = (void *)entry->patched.addr();
  auto size = entry->routing->trampoline_size();
  auto patch = (uint8_t *)entry->routing->trampoline_addr();
  auto expected = entry->enabled ? patch : entry->origin_code_;
  // Do not overwrite another hook or a patch made since Prepare/Disable.
  if (memcmp(address, expected, size) != 0)
    return -1;
  if (entry->enabled == enabled)
    return 0;
  int result = DobbyCodePatch(address, enabled ? patch : entry->origin_code_, (uint32_t)size);
  // A post-write protection/cache error must retain the entry in its actual state.
  if (result == 0 || result == -2)
    entry->enabled = enabled;
  return result;
}
} // namespace

PUBLIC int DobbyPrepare(void *address, void *replacement, void **original) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  return prepare(address, replacement, original);
}

PUBLIC int DobbyCommit(void *address) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  return set_enabled(gInterceptor.find(entry_address(address)), true);
}

PUBLIC int DobbyEnable(void *address) {
  return DobbyCommit(address);
}

PUBLIC int DobbyDisable(void *address) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  return set_enabled(gInterceptor.find(entry_address(address)), false);
}

PUBLIC int DobbyHook(void *address, void *replacement, void **original) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  int result = prepare(address, replacement, original);
  if (result != 0)
    return result;
  // Publish original before activating the entry. Quiescence is still required.
  auto entry = gInterceptor.find(entry_address(address));
  result = set_enabled(entry, true);
  if (result == -1) {
    gInterceptor.remove(entry_address(address));
    delete_entry(entry);
    if (original) *original = nullptr;
  }
  return result;
}

PUBLIC int DobbyInstrument(void *address, dobby_instrument_callback_t callback) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  if (!address || !callback || gInterceptor.find(entry_address(address)))
    return -1;
  features::apple::arm64e_pac_strip(address);
  auto entry = new Interceptor::Entry((addr_t)address);
  entry->pre_handler = callback;
  auto routing = new InstrumentRouting(entry, callback);
  entry->routing = routing;
  routing->BuildRouting();
  if (routing->error) {
    delete_entry(entry);
    return -1;
  }
  gInterceptor.add(entry);
  auto result = set_enabled(entry, true);
  if (result == -1) {
    gInterceptor.remove(entry_address(address));
    delete_entry(entry);
  }
  return result;
}

PUBLIC int DobbyDestroy(void *address) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  auto key = entry_address(address);
  auto entry = gInterceptor.find(key);
  if (!entry)
    return -1;
  if (entry->enabled) {
    int result = set_enabled(entry, false);
    if (result != 0)
      return result;
  }
  gInterceptor.remove(key);
  delete_entry(entry);
  return 0;
}

PUBLIC const char *DobbyGetVersion() {
  return __DOBBY_BUILD_VERSION__;
}

PUBLIC void dobby_set_options(bool enable_near_trampoline, dobby_alloc_near_code_callback_t callback) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  g_enable_near_trampoline = enable_near_trampoline;
  features::apple::arm64e_pac_strip_and_sign(callback);
  custom_alloc_near_code_handler = callback;
}

PUBLIC void dobby_set_near_trampoline(bool enable) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  g_enable_near_trampoline = enable;
}

PUBLIC void dobby_register_alloc_near_code_callback(dobby_alloc_near_code_callback_t callback) {
  std::lock_guard<std::mutex> guard(interceptor_mutex);
  features::apple::arm64e_pac_strip_and_sign(callback);
  custom_alloc_near_code_handler = callback;
}
