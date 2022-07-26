#ifndef UALLOCATOR_ALLOCATOR_H
#define UALLOCATOR_ALLOCATOR_H

#include "hierarchy/back_end/allocator_back_end.h"
#include "hierarchy/front_end/allocator_front_end.h"

namespace UAllocator {
using Allocator = Detail::FrontEnd;
}
#endif