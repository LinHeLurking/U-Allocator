#ifndef UALLOCATOR_ALLOCATOR_H
#define UALLOCATOR_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

namespace UAllocator {
namespace Detail {
// class BackEnd;
class FrontEnd {
 public:
  static void *alloc(size_t size) { return malloc(size); }
  static void dealloc(void *ptr) { free(ptr); }
};
}  // namespace Detail

using Allocator = Detail::FrontEnd;
}  // namespace UAllocator
#endif