#include "dobby/common.h"
#include "PlatformUnifiedInterface/ExecMemory/ClearCacheTool.h"
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <unistd.h>
#include <vector>

PUBLIC int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t size) {
  auto start = (uintptr_t)address;
  if (!address || !buffer || !size || size > UINTPTR_MAX - start)
    return -1;
  auto page_size = (uintptr_t)sysconf(_SC_PAGESIZE);
  auto first = start - start % page_size;
  auto last = start + size - 1 - (start + size - 1) % page_size;
  struct Page { mach_vm_address_t address; vm_prot_t protection; };
  std::vector<Page> pages;
  auto task = mach_task_self();
  for (auto cursor = first;; cursor += page_size) {
    mach_vm_address_t region = cursor;
    mach_vm_size_t region_size = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;
    auto result = mach_vm_region(task, &region, &region_size, VM_REGION_BASIC_INFO_64,
                                 (vm_region_info_t)&info, &count, &object);
    if (MACH_PORT_VALID(object)) mach_port_deallocate(task, object);
    if (result != KERN_SUCCESS || region > cursor || !(info.protection & VM_PROT_READ))
      return -1;
    pages.push_back({cursor, info.protection});
    if (cursor == last) break;
  }
  size_t writable = 0;
  for (; writable < pages.size(); ++writable) {
    auto &page = pages[writable];
    // Darwin enforces W^X; COPY supports private modification of file-backed text.
    auto access = (page.protection | VM_PROT_WRITE | VM_PROT_COPY) & ~VM_PROT_EXECUTE;
    if (mach_vm_protect(task, page.address, page_size, false, access) != KERN_SUCCESS)
      break;
  }
  bool patched = writable == pages.size();
  if (patched) {
    memmove(address, buffer, size);
    ClearCache(address, (void *)(start + size));
  }
  bool restored = true;
  while (writable) {
    auto &page = pages[--writable];
    if (mach_vm_protect(task, page.address, page_size, false, page.protection) != KERN_SUCCESS)
      restored = false;
  }
  return !patched ? -1 : (restored ? 0 : -2);
}
