#ifndef UALLOCATOR_ALLOCATOR_FRONT_END_H
#define UALLOCATOR_ALLOCATOR_FRONT_END_H

#include <stddef.h>

namespace UAllocator {
namespace Detail {

class BackEnd;
class FrontEnd {
 public:
  FrontEnd() = default;
  void *alloc(size_t);
  void dealloc(void *);

 protected:
  BackEnd *back_end;
};

}  // namespace Detail
}  // namespace UAllocator

#endif