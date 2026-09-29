#pragma once

#include "dobby/common.h"
#include "InterceptRouting/InterceptRouting.h"
#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"

struct InlineHookRouting : InterceptRouting {
  addr_t fake_func;

  InlineHookRouting(Interceptor::Entry *entry, addr_t fake_func) : InterceptRouting(entry), fake_func(fake_func) {
  }

  ~InlineHookRouting() = default;

  addr_t TrampolineTarget() override {
    return fake_func;
  }

  void BuildRouting() {
    __FUNC_CALL_TRACE__();

    if (!GenerateTrampoline())
      return;

    GenerateRelocatedCode();

    if (!error)
      BackupOriginCode();
  }
};
