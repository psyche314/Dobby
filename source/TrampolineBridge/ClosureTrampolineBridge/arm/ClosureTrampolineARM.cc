#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_ARM)
#include "dobby/dobby_internal.h"
#include "core/assembler/assembler-arm.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"
using namespace zz::arm;
ClosureTrampoline *GenerateClosureTrampoline(void *data, void *handler) {
  if (!closure_bridge_addr) closure_bridge_init();
  auto entry = new ClosureTrampoline(TRAMPOLINE_UNKNOWN, {}, data, handler);
  TurboAssembler assembler(nullptr);
  PseudoLabel entry_label, bridge_label;
  assembler.Ldr(r12, &entry_label);
  assembler.Ldr(pc, &bridge_label);
  assembler.bindLabel(&entry_label);
  assembler.EmitAddress((addr_t)entry);
  assembler.bindLabel(&bridge_label);
  assembler.EmitAddress((addr_t)closure_bridge_addr);
  entry->buffer = AssemblerCodeBuilder::FinalizeFromTurboAssembler(&assembler);
  return entry;
}
#endif
