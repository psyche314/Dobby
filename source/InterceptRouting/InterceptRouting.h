#pragma once

#include "Interceptor.h"
#include "MemoryAllocator/AssemblerCodeBuilder.h"
#include "InstructionRelocation/InstructionRelocation.h"
#include "TrampolineBridge/Trampoline/Trampoline.h"
#include "RoutingPlugin.h"
#include "NearBranchTrampoline/NearBranchTrampoline.h"

Trampoline *GenerateNearTrampolineBuffer(addr_t src, addr_t dst);
Trampoline *GenerateNormalTrampolineBuffer(addr_t from, addr_t to);

struct InterceptRouting {
  Interceptor::Entry *entry = 0;
  Trampoline *trampoline = 0;
  Trampoline *near_trampoline = 0;
  int error = 0;

  explicit InterceptRouting(Interceptor::Entry *entry) : entry(entry) {
  }

  virtual ~InterceptRouting() {
    if (trampoline) {
      delete trampoline->forward_trampoline;
      operator delete((void *)trampoline->addr());
      delete trampoline;
    }
    if (near_trampoline) {
      delete near_trampoline->forward_trampoline;
      operator delete((void *)near_trampoline->addr());
      delete near_trampoline;
    }
  }

  virtual addr_t TrampolineTarget() {
    UNREACHABLE();
    return -1;
  }

  addr_t trampoline_addr() {
    if (near_trampoline)
      return near_trampoline->addr();
    return trampoline ? trampoline->addr() : 0;
  }

  size_t trampoline_size() {
    if (near_trampoline)
      return near_trampoline->size();
    return trampoline ? trampoline->size() : 0;
  }

  bool GenerateTrampoline() {
    __FUNC_CALL_TRACE__();
    addr_t from = entry->addr;
    // The ARM generator needs the Thumb tag to select its instruction encoding.

    addr_t to = TrampolineTarget();

    if (g_enable_near_trampoline) {
      near_trampoline = GenerateNearTrampolineBuffer(from, to);
    }

    if (!near_trampoline) {
      trampoline = GenerateNormalTrampolineBuffer(from, to);
    }
    error = !trampoline_addr();
    return !error;
  }

  void GenerateRelocatedCode() {
    __FUNC_CALL_TRACE__();
    if (trampoline_addr() == 0) {
      ERROR_LOG("GenerateTrampoline must be called first");
      error = 1;
      return;
    }

    auto code_addr = entry->addr;
    features::arm_thumb_fix_addr(code_addr);
    auto preferred_size = trampoline_size();
    auto origin = CodeMemBlock(code_addr, preferred_size);
    auto relocated = CodeMemBlock(0, 0);
    // Keep the Thumb tag for the relocator, but read and patch the aligned entry.
    GenRelocateCodeAndBranch((void *)entry->addr, &origin, &relocated);
    if (relocated.size == 0) {
      error = 1;
      return;
    }
    DEBUG_LOG("origin: %p, size: %d", origin.addr(), origin.size);
    debug_hex_log_buffer((uint8_t *)origin.addr(), origin.size);
    DEBUG_LOG("relocated: %p, size: %d", relocated.addr(), relocated.size);
    debug_hex_log_buffer((uint8_t *)relocated.addr(), relocated.size);

    entry->patched = origin;
    entry->relocated = relocated;
  }

  void BackupOriginCode() {
    __FUNC_CALL_TRACE__();
    entry->backup_orig_code();
  }
};
