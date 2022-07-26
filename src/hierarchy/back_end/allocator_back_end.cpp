#include "allocator_back_end.h"

#include <stdlib.h>

#include "../front_end/allocator_front_end.h"

namespace UAllocator {
namespace Detail {
// Currently just delegate back-end to system malloc & free;
void *BackEnd::alloc(size_t size) { return malloc(size); }
void BackEnd::dealloc(void *ptr) { free(ptr); }
}  // namespace Detail
}  // namespace UAllocator