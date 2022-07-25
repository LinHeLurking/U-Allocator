#include "allocator.h"

#include <stdlib.h>

namespace UAllocator {
namespace Detail {
// Currently just delegate front-end to back-end;
void *FrontEnd::alloc(size_t size) { return back_end->alloc(size); }
void FrontEnd::dealloc(void *ptr) { back_end->dealloc(ptr); }

// Currently just delegate back-end to system malloc & free;
void *BackEnd::alloc(size_t size) { return malloc(size); }
void BackEnd::dealloc(void *ptr) { free(ptr); }

}  // namespace Detail
}  // namespace UAllocator