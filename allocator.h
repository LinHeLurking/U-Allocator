#ifndef U_ALLOCATOR_ALLOCATOR_H
#define U_ALLOCATOR_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

namespace UAllocator {

namespace Detail {
// Forward declaration;
class BackEnd;

class FrontEnd {
 public:
  FrontEnd() = default;
  void *alloc(size_t);
  void dealloc(void *);

 protected:
  BackEnd *back_end;
};
class BackEnd {
 public:
  BackEnd() = default;
  void *alloc(size_t);
  void dealloc(void *);

 protected:
  FrontEnd *front_end;
};
}  // namespace Detail

using Allocator = Detail::FrontEnd;

}  // namespace UAllocator

#endif  // U_ALLOCATOR_ALLOCATOR_H