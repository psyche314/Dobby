#include <stdio.h>

#include <windows.h>

#include "logging/logging.h"
#include "logging/check_logging.h"
#include "PlatformUnifiedInterface/platform.h"

static DWORD GetProtectionFromMemoryPermission(MemoryPermission access) {
  switch (access) {
  case kRead: return PAGE_READONLY;
  case kReadWrite: return PAGE_READWRITE;
  case kExecute: return PAGE_EXECUTE;
  case kReadExecute: return PAGE_EXECUTE_READ;
  case kReadWriteExecute: return PAGE_EXECUTE_READWRITE;
  default: return PAGE_NOACCESS;
  }
}

int OSMemory::PageSize() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwPageSize;
}

void *OSMemory::Allocate(size_t size, MemoryPermission access) {
  return Allocate(size, access, nullptr);
}

void *OSMemory::Allocate(size_t size, MemoryPermission access, void *address) {
  return VirtualAlloc(address, size, MEM_RESERVE | MEM_COMMIT, GetProtectionFromMemoryPermission(access));
}

bool OSMemory::Free(void *address, size_t size) {
  return VirtualFree(address, 0, MEM_RELEASE) != 0;
}

bool OSMemory::Release(void *address, size_t size) {
  return Free(address, size);
}

bool OSMemory::SetPermission(void *address, size_t size, MemoryPermission access) {
  DWORD previous;
  return VirtualProtect(address, size, GetProtectionFromMemoryPermission(access), &previous) != 0;
}

// =====

void OSPrint::Print(const char *format, ...) {
  va_list args;
  va_start(args, format);
  VPrint(format, args);
  va_end(args);
}

void OSPrint::VPrint(const char *format, va_list args) {
  vprintf(format, args);
}
