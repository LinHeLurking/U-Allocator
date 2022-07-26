#include "allocator_front_end.h"

#include "../back_end/allocator_back_end.h"

namespace UAllocator {
namespace Detail {
// Currently just delegate front-end to back-end;
void *FrontEnd::alloc(size_t size) { return back_end->alloc(size); }
void FrontEnd::dealloc(void *ptr) { back_end->dealloc(ptr); }
}  // namespace Detail
}  // namespace UAllocator