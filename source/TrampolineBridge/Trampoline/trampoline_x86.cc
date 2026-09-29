#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)
#include "dobby/dobby_internal.h"
Trampoline *GenerateNormalTrampolineBuffer(addr_t from, addr_t to) {
  CodeMemBuffer code;
  code.Emit<uint8_t>(0xe9);
  code.Emit<uint32_t>(to - (from + 5));
  return new Trampoline(TRAMPOLINE_UNKNOWN, code.dup());
}
Trampoline *GenerateNearTrampolineBuffer(addr_t from, addr_t to) { return nullptr; }
#endif
