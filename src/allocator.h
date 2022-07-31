#ifndef UALLOCATOR_ALLOCATOR_H
#define UALLOCATOR_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

#include <bitset>
#include <list>
#include <map>
#include <vector>

namespace UAllocator {
namespace Detail {

class AllocatorBase {
 public:
  AllocatorBase() = default;
  ~AllocatorBase() = default;
  static void *allocate(size_t size) { return malloc(size); }
  static void deallocate(void *ptr) { free(ptr); }
};

template <typename BackEndAllocator = AllocatorBase>
class FrontEnd {
 protected:
 public:
  FrontEnd() {}

  void *allocate(size_t size) { return nullptr; }
  void deallocate(void *ptr) {}
};
}  // namespace Detail

using Allocator = Detail::FrontEnd<>;
}  // namespace UAllocator
#endif