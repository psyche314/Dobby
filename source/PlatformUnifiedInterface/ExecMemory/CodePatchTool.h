#pragma once
#include "dobby.h"

#if defined(__linux__)
int PatchExecutableCode(void *address, uint8_t *buffer, uint32_t size);
#else
inline int PatchExecutableCode(void *address, uint8_t *buffer, uint32_t size) {
  return DobbyCodePatch(address, buffer, size);
}
#endif
