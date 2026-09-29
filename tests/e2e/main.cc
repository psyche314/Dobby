#include "dobby.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#endif
#endif
#if defined(DOBBY_E2E_JNI)
#include <jni.h>
#include <android/log.h>
#endif

#define CHECK(expression) do { if (!(expression)) {   std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression);   std::fflush(stderr); std::abort(); } } while (0)

static void report(const char *message) {
  std::puts(message);
  std::fflush(stdout);
#if defined(DOBBY_E2E_JNI)
  __android_log_write(ANDROID_LOG_INFO, "DobbyE2E", message);
#endif
}

struct Mapping {
  uint8_t *data;
  size_t page_size;
  size_t size;
  explicit Mapping(size_t pages) {
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    page_size = info.dwPageSize;
    size = pages * page_size;
    data = (uint8_t *)VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    page_size = sysconf(_SC_PAGESIZE);
    size = pages * page_size;
    data = (uint8_t *)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(data != MAP_FAILED);
#endif
    CHECK(data);
  }
  ~Mapping() {
#if defined(_WIN32)
    VirtualFree(data, 0, MEM_RELEASE);
#else
    munmap(data, size);
#endif
  }
  void protect(size_t index, bool write, bool execute) {
#if defined(_WIN32)
    DWORD previous;
    DWORD perm = execute ? (write ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ) :
                           (write ? PAGE_READWRITE : PAGE_READONLY);
    CHECK(VirtualProtect(data + index * page_size, page_size, perm, &previous));
#else
    CHECK(mprotect(data + index * page_size, page_size,
                   PROT_READ | (write ? PROT_WRITE : 0) | (execute ? PROT_EXEC : 0)) == 0);
#endif
  }
  int protection(size_t index) {
#if defined(_WIN32)
    MEMORY_BASIC_INFORMATION info;
    CHECK(VirtualQuery(data + index * page_size, &info, sizeof(info)));
    return info.Protect;
#else
#if defined(__APPLE__)
    mach_vm_address_t address = (uintptr_t)(data + index * page_size);
    mach_vm_size_t length = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;
    auto result = mach_vm_region(mach_task_self(), &address, &length, VM_REGION_BASIC_INFO_64,
                                (vm_region_info_t)&info, &count, &object);
    if (MACH_PORT_VALID(object)) mach_port_deallocate(mach_task_self(), object);
    CHECK(result == KERN_SUCCESS);
    return info.protection;
#else
    FILE *file = fopen("/proc/self/maps", "r");
    CHECK(file);
    char line[4096];
    auto address = (uintptr_t)(data + index * page_size);
    int result = -1;
    while (fgets(line, sizeof(line), file)) {
      unsigned long long start, end;
      char perm[5];
      if (sscanf(line, "%llx-%llx %4s", &start, &end, perm) == 3 && address >= start && address < end) {
        result = (perm[0] == 'r') | ((perm[1] == 'w') << 1) | ((perm[2] == 'x') << 2);
        break;
      }
    }
    fclose(file);
    return result;
#endif
#endif
  }
};

using Function = int (*)();
static Function original;
static int replacement() { return original() + 100; }
static int constant_replacement() { return 91; }
static int callbacks;
static void instrument(void *, DobbyRegisterContext *) { ++callbacks; }

static void make_function(uint8_t *address) {
#if defined(__aarch64__)
  const uint32_t code[] = {0x528000e0, 0xd65f03c0, 0xd503201f, 0xd503201f, 0xd503201f, 0xd503201f};
#elif defined(__arm__)
  const uint32_t code[] = {0xe3a00007, 0xe12fff1e, 0xe320f000, 0xe320f000, 0xe320f000, 0xe320f000};
#else
  const uint8_t code[] = {0xb8,7,0,0,0,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
#endif
  std::memcpy(address, code, sizeof(code));
}

static void lifecycle(bool near_branch, bool thumb = false) {
  Mapping memory(2);
  auto target = memory.data + memory.page_size - (thumb ? 2 : 4);
  make_function(target);
#if defined(__arm__)
  if (thumb) {
    const uint16_t code[] = {0x2007,0x4770,0xbf00,0xbf00,0xbf00,0xbf00,0xbf00,0xbf00};
    std::memcpy(target, code, sizeof(code));
  }
#endif
  auto hook_address = (void *)((uintptr_t)target | (thumb ? 1 : 0));
  memory.protect(0, false, true);
  memory.protect(1, false, true);
  // Also synchronizes the instruction cache before the first call on ARM.
  uint8_t before[32];
  std::memcpy(before, target, sizeof(before));
#if defined(_WIN32)
  CHECK(FlushInstructionCache(GetCurrentProcess(), target, sizeof(before)));
#else
  __builtin___clear_cache((char *)target, (char *)target + sizeof(before));
#endif
  auto function = (Function)hook_address;
  CHECK(function() == 7);
  dobby_set_near_trampoline(near_branch);
  CHECK(DobbyPrepare(hook_address, (void *)replacement, (void **)&original) == 0);
  CHECK(original && original() == 7);
  CHECK(function() == 7 && std::memcmp(before, target, sizeof(before)) == 0);
  void *duplicate = (void *)uintptr_t(1);
  CHECK(DobbyPrepare(hook_address, (void *)replacement, &duplicate) != 0 && duplicate == (void *)uintptr_t(1));
  CHECK(DobbyDisable(hook_address) == 0);
  CHECK(DobbyCommit(hook_address) == 0);
  CHECK(DobbyCommit(hook_address) == 0);
  CHECK(function() == 107);
  auto saved_original = original;
  for (int i = 0; i != 100; ++i) {
    CHECK(DobbyDisable(hook_address) == 0 && DobbyDisable(hook_address) == 0);
    CHECK(function() == 7 && original() == 7);
    CHECK(DobbyEnable(hook_address) == 0 && DobbyEnable(hook_address) == 0);
    CHECK(function() == 107 && original == saved_original);
  }
  CHECK(DobbyDestroy(hook_address) == 0);
  CHECK(function() == 7 && std::memcmp(before, target, sizeof(before)) == 0);
  CHECK(DobbyDestroy(hook_address) != 0 && DobbyEnable(hook_address) != 0);
  CHECK(DobbyPrepare(hook_address, (void *)replacement, (void **)&original) == 0);
  CHECK(DobbyDestroy(hook_address) == 0); // cancel a prepared hook
  CHECK(function() == 7);
  CHECK(DobbyHook(hook_address, (void *)constant_replacement, nullptr) == 0);
  CHECK(function() == 91);
  CHECK(DobbyDestroy(hook_address) == 0);

  CHECK(DobbyPrepare(hook_address, (void *)replacement, (void **)&original) == 0);
  uint8_t changed = before[0] ^ 1;
  CHECK(DobbyCodePatch(target, &changed, 1) == 0);
  CHECK(DobbyCommit(hook_address) != 0 && target[0] == changed);
  CHECK(DobbyCodePatch(target, before, 1) == 0);
  CHECK(DobbyDestroy(hook_address) == 0);

  callbacks = 0;
  CHECK(DobbyInstrument(hook_address, instrument) == 0);
  CHECK(function() == 7 && callbacks == 1);
  CHECK(DobbyDisable(hook_address) == 0);
  CHECK(function() == 7 && callbacks == 1);
  CHECK(DobbyEnable(hook_address) == 0);
  CHECK(function() == 7 && callbacks == 2);
  CHECK(DobbyDestroy(hook_address) == 0 && function() == 7);
  report(near_branch ? "PASS near hook lifecycle and instrumentation" : "PASS far hook lifecycle and instrumentation");
}

static void patch_pages() {
  Mapping memory(4);
  std::memset(memory.data, 0x11, memory.size);
  memory.protect(0, false, true);
#if defined(__APPLE__)
  memory.protect(1, true, false); // ARM64 macOS rejects simultaneous W+X.
#else
  memory.protect(1, true, true);
#endif
  memory.protect(2, false, false);
  memory.protect(3, true, false);
  int before[4];
  for (size_t i = 0; i != 4; ++i) before[i] = memory.protection(i);
  std::vector<uint8_t> patch(memory.page_size * 2 + 6, 0x55);
  auto target = memory.data + memory.page_size - 3;
  CHECK(DobbyCodePatch(target, patch.data(), (uint32_t)patch.size()) == 0);
  CHECK(std::memcmp(target, patch.data(), patch.size()) == 0);
  CHECK(target[-1] == 0x11 && target[patch.size()] == 0x11);
  for (size_t i = 0; i != 4; ++i) CHECK(memory.protection(i) == before[i]);
  CHECK(DobbyCodePatch(nullptr, patch.data(), 1) != 0);
  CHECK(DobbyCodePatch(target, nullptr, 1) != 0);
  CHECK(DobbyCodePatch(target, patch.data(), 0) != 0);
  CHECK(DobbyCodePatch((void *)(UINTPTR_MAX - 1), patch.data(), 4) != 0);

  // A hole on the second page must be detected before the first byte changes.
#if defined(_WIN32)
  CHECK(VirtualFree(memory.data + memory.page_size, memory.page_size, MEM_DECOMMIT));
#else
  CHECK(munmap(memory.data + memory.page_size, memory.page_size) == 0);
#endif
  uint8_t previous[3];
  std::memcpy(previous, target, 3);
  CHECK(DobbyCodePatch(target, patch.data(), 6) != 0);
  CHECK(std::memcmp(previous, target, 3) == 0);
  CHECK(memory.protection(0) == before[0]);
  // Ending exactly at a boundary must not touch the unmapped following page.
  CHECK(DobbyCodePatch(target, patch.data(), 3) == 0);
  report("PASS four-page patch, original permissions, unmapped failure, exact boundary");
}

static int run() {
  report("Dobby native E2E starting");
  { Mapping probe(1); printf("System page size: %zu bytes\n", probe.page_size); }
  CHECK(DobbyGetVersion() && *DobbyGetVersion());
  CHECK(DobbyPrepare(nullptr, (void *)replacement, nullptr) != 0);
  CHECK(DobbyCommit(nullptr) != 0 && DobbyDisable(nullptr) != 0);
  patch_pages();
  lifecycle(false);
  lifecycle(true);
#if defined(__arm__)
  lifecycle(false, true);
  report("PASS Thumb address normalization and aligned trampoline");
#endif
  report("PASS all Dobby native E2E checks");
  return 0;
}
#if defined(DOBBY_E2E_JNI)
extern "C" JNIEXPORT jint JNICALL Java_org_psyche_dobby_E2EActivity_runTests(JNIEnv *, jclass) { return run(); }
#else
int main() { return run(); }
#endif
