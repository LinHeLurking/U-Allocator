#ifndef UALLOCATOR_ALLOCATOR_H
#define UALLOCATOR_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

namespace UAllocator {
namespace Detail {

class AllocatorBase {
 public:
  AllocatorBase() = default;
  virtual ~AllocatorBase() = default;
  virtual void *alloc(size_t size) { return malloc(size); }
  virtual void dealloc(void *ptr) { free(ptr); }
};
class FrontEnd : public AllocatorBase {};
}  // namespace Detail

using Allocator = Detail::FrontEnd;
}  // namespace UAllocator
#endif