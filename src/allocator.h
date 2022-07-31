#ifndef UALLOCATOR_ALLOCATOR_H
#define UALLOCATOR_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

#include <bitset>
#include <list>
#include <map>
#include <vector>

#include "mem_pool.h"

namespace UAllocator {
namespace Detail {

static thread_local MemPool* cache = MemPool::create();

class AllocatorFrontEnd {
 public:
  AllocatorFrontEnd() = default;
  ~AllocatorFrontEnd() = default;

  inline void* allocate(size_t size) const noexcept {
    return cache->allocate(size);
  }
  inline void deallocate(void* ptr) const noexcept {
    return cache->deallocate(ptr);
  }
};
}  // namespace Detail

using Allocator = Detail::AllocatorFrontEnd;

}  // namespace UAllocator
#endif