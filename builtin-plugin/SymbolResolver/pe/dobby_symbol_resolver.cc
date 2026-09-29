#include "dobby/common.h"
#include <windows.h>

PUBLIC void *DobbySymbolResolver(const char *image_name, const char *symbol_name) {
  if (!symbol_name) return nullptr;
  auto module = GetModuleHandleA(image_name);
  return module ? (void *)GetProcAddress(module, symbol_name) : nullptr;
}
