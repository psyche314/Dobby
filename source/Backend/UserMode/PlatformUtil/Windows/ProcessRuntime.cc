#include "PlatformUtil/ProcessRuntime.h"
#include <windows.h>
#include <tlhelp32.h>

const stl::vector<MemRegion> &ProcessRuntime::getMemoryLayout() {
  static stl::vector<MemRegion> regions;
  regions.clear();
  uintptr_t cursor = 0;
  MEMORY_BASIC_INFORMATION info;
  while (VirtualQuery((void *)cursor, &info, sizeof(info))) {
    auto end = (uintptr_t)info.BaseAddress + info.RegionSize;
    if (end <= cursor) break;
    cursor = end;
    if (info.State == MEM_FREE) continue;
    int permission = 0;
    if (!(info.Protect & PAGE_GUARD)) {
      switch (info.Protect & 0xff) {
      case PAGE_READONLY: permission = kRead; break;
      case PAGE_READWRITE: case PAGE_WRITECOPY: permission = kReadWrite; break;
      case PAGE_EXECUTE: permission = kExecute; break;
      case PAGE_EXECUTE_READ: permission = kReadExecute; break;
      case PAGE_EXECUTE_READWRITE: case PAGE_EXECUTE_WRITECOPY: permission = kReadWriteExecute; break;
      }
    }
    regions.push_back(MemRegion((addr_t)info.BaseAddress, info.RegionSize, permission));
  }
  return regions;
}

const stl::vector<RuntimeModule> &ProcessRuntime::getModuleMap() {
  static stl::vector<RuntimeModule> modules;
  modules.clear();
  auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
  if (snapshot == INVALID_HANDLE_VALUE) return modules;
  MODULEENTRY32 entry{};
  entry.dwSize = sizeof(entry);
  if (Module32First(snapshot, &entry)) {
    do {
      RuntimeModule module{};
      module.base = entry.modBaseAddr;
      strncpy(module.path, entry.szExePath, sizeof(module.path) - 1);
      modules.push_back(module);
    } while (Module32Next(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return modules;
}

RuntimeModule ProcessRuntime::getModule(const char *name) {
  for (const auto &module : getModuleMap())
    if (name && strstr(module.path, name)) return module;
  return {};
}
