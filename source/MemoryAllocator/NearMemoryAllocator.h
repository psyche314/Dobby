#pragma once

#include "dobby/common.h"
#include "MemoryAllocator.h"
#include "PlatformUtil/ProcessRuntime.h"
#include <stdint.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#define KB (1024uLL)
#define MB (1024uLL * KB)
#define GB (1024uLL * MB)

// memmem impl
inline void *memmem_impl(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen) {
  if (!haystack || !needle) {
    return (void *)haystack;
  } else {
    const char *h = (const char *)haystack;
    const char *n = (const char *)needle;
    size_t l = needlelen;
    const char *r = h;
    while (l && (l <= haystacklen)) {
      if (*n++ != *h++) {
        r = h;
        n = (const char *)needle;
        l = needlelen;
      } else {
        --l;
      }
      --haystacklen;
    }
    return l ? nullptr : (void *)r;
  }
}

inline dobby_alloc_near_code_callback_t custom_alloc_near_code_handler = nullptr;
PUBLIC inline void dobby_register_alloc_near_code_callback(dobby_alloc_near_code_callback_t handler) {
  features::apple::arm64e_pac_strip_and_sign(handler);
  custom_alloc_near_code_handler = handler;
}

struct NearMemoryAllocator {
  stl::vector<simple_linear_allocator_t*> code_page_allocators;
  stl::vector<simple_linear_allocator_t*> data_page_allocators;

  inline static NearMemoryAllocator *Shared();

  MemBlock allocNearCodeBlock(uint32_t in_size, addr_t pos, size_t range) {
    if (custom_alloc_near_code_handler) {
      auto addr = custom_alloc_near_code_handler(in_size, pos, range);
      if (addr)
        return {addr, in_size};
    } else {
      auto low = pos > range ? pos - range : 0;
      auto high = range > UINTPTR_MAX - pos ? UINTPTR_MAX : pos + range;
      auto search_range = MemRange(low, high - low);
      return allocNearBlock(in_size, search_range, true);
    }
    return {};
  }

  MemBlock allocNearDataBlock(uint32_t in_size, addr_t pos, size_t range) {
    auto low = pos > range ? pos - range : 0;
      auto high = range > UINTPTR_MAX - pos ? UINTPTR_MAX : pos + range;
      auto search_range = MemRange(low, high - low);
    return allocNearBlock(in_size, search_range, false);
  }

  MemBlock allocNearBlock(uint32_t in_size, MemRange search_range, bool is_exec = true) {
    // step-1: search from allocators first
    auto &allocators = is_exec ? code_page_allocators : data_page_allocators;
    for (auto allocator : allocators) {
      auto cursor = allocator->cursor();
      auto unused_size = allocator->capacity - allocator->size;
      auto unused_range = MemRange((addr_t)cursor, unused_size);
      auto intersect = search_range.intersect(unused_range);
      if (intersect.size < in_size)
        continue;

      auto gap_size = intersect.addr() - (addr_t)cursor;
      if (gap_size) {
        allocator->alloc(gap_size);
      }

      auto result = allocator->alloc(in_size);
      DEBUG_LOG("step-1 allocator: %p, size: %d", (void *)result, in_size);
      return {(addr_t)result, (size_t)in_size};
    }

    // step-2: search from unused page between regions
    auto regions = ProcessRuntime::getMemoryLayout();
    for (int i = 0; i < regions.size(); ++i) {
      auto *region = &regions[i];
      auto *prev_region = i > 0 ? &regions[i - 1] : nullptr;
      auto *next_region = i < regions.size() - 1 ? &regions[i + 1] : nullptr;
      if (!next_region)
        break;

      auto unused_region_start = region->end();
      auto unused_region_size = next_region->addr() - region->end();
      MemRegion unused_region(unused_region_start, unused_region_size, kNoAccess);
      auto intersect = search_range.intersect(unused_region);
      if (intersect.size < in_size)
        continue;

      size_t granularity = OSMemory::PageSize();
#if defined(_WIN32)
      SYSTEM_INFO info;
      GetSystemInfo(&info);
      granularity = info.dwAllocationGranularity;
#endif
      auto candidate = ALIGN_CEIL(intersect.addr(), granularity);
      if (candidate < intersect.addr() || candidate >= intersect.end() ||
          OSMemory::PageSize() > intersect.end() - candidate)
        continue;
      auto page = OSMemory::Allocate(OSMemory::PageSize(), is_exec ? kReadExecute : kReadWrite, (void *)candidate);
      if (!page)
        continue;
      auto allocator = new simple_linear_allocator_t((uint8_t *)page, OSMemory::PageSize());
      if (is_exec)
        code_page_allocators.push_back(allocator);
      else
        data_page_allocators.push_back(allocator);

      // should be fallthrough to step-1 allocator
      return allocNearBlock(in_size, search_range, is_exec);
    }

    // Zero-filled bytes in somebody else's executable mapping are not free memory.

    return {};
  }
};

inline NearMemoryAllocator gNearMemoryAllocator;
NearMemoryAllocator *NearMemoryAllocator::Shared() {
  return &gNearMemoryAllocator;
}