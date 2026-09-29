#include <stddef.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <libkern/OSCacheControl.h>
#endif

void ClearCache(void *start, void *end) {
#if defined(_WIN32)
  FlushInstructionCache(GetCurrentProcess(), start, (char *)end - (char *)start);
#elif defined(__APPLE__)
  sys_icache_invalidate(start, (char *)end - (char *)start);
#else
  __builtin___clear_cache((char *)start, (char *)end);
#endif
}
