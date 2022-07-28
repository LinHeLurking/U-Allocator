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
  virtual void *allocate(size_t size) { return malloc(size); }
  virtual void deallocate(void *ptr) { free(ptr); }
};
class FrontEnd : public AllocatorBase {
 public:
  FrontEnd();

  void *allocate(size_t size) override;
  void deallocate(void *ptr) override;

 protected:
  AllocatorBase *back_end = nullptr;
  void refill(int slot_id);
};
}  // namespace Detail

using Allocator = Detail::FrontEnd;
}  // namespace UAllocator
#endif