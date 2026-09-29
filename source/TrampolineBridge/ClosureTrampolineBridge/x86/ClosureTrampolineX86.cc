#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)
#include "dobby/dobby_internal.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"
ClosureTrampoline *GenerateClosureTrampoline(void *data, void *handler) {
  if (!closure_bridge_addr) closure_bridge_init();
  auto block = gMemoryAllocator.allocExecBlock(10);
  if (!block.addr()) return nullptr;
  auto entry = new ClosureTrampoline(TRAMPOLINE_UNKNOWN, block, data, handler);
  uint8_t code[10] = {0x68,0,0,0,0,0xe9,0,0,0,0};
  auto entry_address = (uint32_t)entry;
  auto displacement = (uint32_t)closure_bridge_addr - (block.addr() + sizeof(code));
  memcpy(code + 1, &entry_address, 4);
  memcpy(code + 6, &displacement, 4);
  if (DobbyCodePatch((void *)block.addr(), code, sizeof(code)) != 0) {
    delete entry;
    return nullptr;
  }
  return entry;
}
#endif
