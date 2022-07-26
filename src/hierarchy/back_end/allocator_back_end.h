#ifndef UALLOCATOR_ALLOCATOR_BACK_END_H
#define UALLOCATOR_ALLOCATOR_BACK_END_H

#include <stddef.h>

namespace UAllocator {

namespace Detail {
class FrontEnd;
class BackEnd {
 public:
  BackEnd() = default;
  void *alloc(size_t);
  void dealloc(void *);

 protected:
  FrontEnd *front_end;
};
}  // namespace Detail
}  // namespace UAllocator

#endif