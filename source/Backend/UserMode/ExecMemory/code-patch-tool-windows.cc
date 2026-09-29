#include "dobby/common.h"
#include <windows.h>
#include <vector>

PUBLIC int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t size) {
  auto start = (uintptr_t)address;
  if (!address || !buffer || !size || size > UINTPTR_MAX - start)
    return -1;
  SYSTEM_INFO system;
  GetSystemInfo(&system);
  auto page_size = (uintptr_t)system.dwPageSize;
  auto first = start - start % page_size;
  auto last = start + size - 1 - (start + size - 1) % page_size;
  struct Page { uintptr_t address; DWORD protection; };
  std::vector<Page> pages;
  for (auto cursor = first;; cursor += page_size) {
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery((void *)cursor, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
      return -1;
    pages.push_back({cursor, info.Protect});
    if (cursor == last) break;
  }

  size_t writable = 0;
  for (; writable < pages.size(); ++writable) {
    auto &page = pages[writable];
    DWORD previous;
    DWORD access = (page.protection & 0xf0) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    if (!VirtualProtect((void *)page.address, page_size, access, &previous))
      break;
    page.protection = previous;
  }
  bool patched = writable == pages.size();
  bool restored = true;
  if (patched) {
    memmove(address, buffer, size);
    restored = FlushInstructionCache(GetCurrentProcess(), address, size) != 0;
  }
  while (writable) {
    auto &page = pages[--writable];
    DWORD previous;
    if (!VirtualProtect((void *)page.address, page_size, page.protection, &previous))
      restored = false;
  }
  return !patched ? -1 : (restored ? 0 : -2);
}
