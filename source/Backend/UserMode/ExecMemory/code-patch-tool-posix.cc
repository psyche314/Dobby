#include "dobby/common.h"
#include "PlatformUnifiedInterface/ExecMemory/ClearCacheTool.h"
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

PUBLIC int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t size) {
  auto start = (uintptr_t)address;
  if (!address || !buffer || !size || size > UINTPTR_MAX - start)
    return -1;
  auto end = start + size;
  auto page_size = (uintptr_t)sysconf(_SC_PAGESIZE);
  if (!page_size || page_size == UINTPTR_MAX)
    return -1;
  auto first = start - start % page_size;
  auto last = end - 1 - (end - 1) % page_size;
  struct Page { uintptr_t address; int protection; };
  std::vector<Page> pages;
  FILE *maps = fopen("/proc/self/maps", "r");
  if (!maps)
    return -1;
  char line[4096];
  uintptr_t cursor = first;
  bool complete = false;
  while (fgets(line, sizeof(line), maps)) {
    uintptr_t low, high;
    char perm[5];
    if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %4s", &low, &high, perm) != 3)
      continue;
    if (cursor < low)
      break; // An unmapped page must fail before changing any bytes.
    if (cursor >= high)
      continue;
    int protection = (perm[0] == 'r' ? PROT_READ : 0) |
                     (perm[1] == 'w' ? PROT_WRITE : 0) |
                     (perm[2] == 'x' ? PROT_EXEC : 0);
    if (!(protection & PROT_READ))
      break;
    while (cursor < high) {
      pages.push_back({cursor, protection});
      if (cursor == last) { complete = true; break; }
      cursor += page_size;
    }
    if (complete) break;
  }
  fclose(maps);
  if (!complete)
    return -1;

  size_t writable = 0;
  for (; writable < pages.size(); ++writable) {
    auto &page = pages[writable];
    if (mprotect((void *)page.address, page_size, page.protection | PROT_WRITE) != 0)
      break;
  }
  bool patched = writable == pages.size();
  if (patched) {
    memmove(address, buffer, size);
    ClearCache(address, (void *)end);
  }
  bool restored = true;
  while (writable) {
    auto &page = pages[--writable];
    if (mprotect((void *)page.address, page_size, page.protection) != 0)
      restored = false;
  }
  // -2 means the bytes were written but restoring permissions failed.
  return !patched ? -1 : (restored ? 0 : -2);
}
