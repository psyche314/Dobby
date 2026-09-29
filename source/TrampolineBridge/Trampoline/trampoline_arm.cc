#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_ARM)
#include "dobby/dobby_internal.h"
Trampoline *GenerateNormalTrampolineBuffer(addr_t from, addr_t to) {
  CodeMemBuffer code;
  if (from & 1) {
    from &= ~1u;
    if (from & 2) code.Emit<uint16_t>(0xbf00); // align the literal to four bytes
    code.Emit<uint16_t>(0xf8df); // ldr.w pc, [pc]
    code.Emit<uint16_t>(0xf000);
  } else {
    code.Emit<uint32_t>(0xe51ff004); // ldr pc, [pc, #-4]
  }
  code.Emit<uint32_t>(to);
  return new Trampoline(TRAMPOLINE_UNKNOWN, code.dup());
}
Trampoline *GenerateNearTrampolineBuffer(addr_t from, addr_t to) { return nullptr; }
#endif
