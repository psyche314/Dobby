#pragma once

#include "dobby/common.h"
#include "InterceptRouting/InterceptRouting.h"
#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"

struct InstrumentRouting : InterceptRouting {
  ClosureTrampoline *instrument_tramp = nullptr;

  InstrumentRouting(Interceptor::Entry *entry, dobby_instrument_callback_t pre_handler) : InterceptRouting(entry) {
  }

  ~InstrumentRouting() {
    if (instrument_tramp) {
      // TODO: free code block
      delete instrument_tramp;
    }
  }

  addr_t TrampolineTarget() override {
    return instrument_tramp->addr();
  }

  void GenerateInstrumentClosureTrampoline() {
    __FUNC_CALL_TRACE__();
    instrument_tramp = ::GenerateInstrumentClosureTrampoline(entry);
  }

  void BuildRouting() {
    __FUNC_CALL_TRACE__();

    GenerateInstrumentClosureTrampoline();

    if (!instrument_tramp || !instrument_tramp->addr() || !GenerateTrampoline()) {
      error = 1;
      return;
    }

    GenerateRelocatedCode();

    if (!error)
      BackupOriginCode();
  }
};
