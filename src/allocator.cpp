#include "allocator.h"

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>

template <typename Subscriptable, size_t N>
struct is_all_positive {
  static constexpr bool check(Subscriptable a) {
    return a[N - 1] > 0 && is_all_positive<Subscriptable, N - 1>::check(a);
  }
};

template <typename Subscriptable>
struct is_all_positive<Subscriptable, 0> {
  static constexpr bool check(Subscriptable a) { return true; }
};

template <typename Int32,
          typename std::enable_if<std::is_integral<Int32>::value &&
                                      sizeof(Int32) == 4,
                                  bool>::type = true>
Int32 round2pow(Int32 x) {
  return 1 << (32 - 1 - __builtin_clz(x));
}

template <typename Int64,
          typename std::enable_if<std::is_integral<Int64>::value &&
                                      sizeof(Int64) == 8,
                                  bool>::type = true>
Int64 round2pow(Int64 x) {
  return 1UL << (64 - 1 - __builtin_clzll(x));
}

template <typename Int64,
          typename std::enable_if<std::is_integral<Int64>::value &&
                                      sizeof(Int64) == 8,
                                  bool>::type = true>
Int64 ctz(Int64 x) {
  return __builtin_ctzll(x);
}

template <typename Int32,
          typename std::enable_if<std::is_integral<Int32>::value &&
                                      sizeof(Int32) == 4,
                                  bool>::type = true>
Int32 ctz(Int32 x) {
  return __builtin_ctz(x);
}

namespace UAllocator {
namespace Detail {
namespace FrontEndConfig {
constexpr size_t SIZE_NUM = 11;
constexpr size_t CHUNK_NUM = 1024;
}  // namespace FrontEndConfig

using namespace FrontEndConfig;

thread_local static std::list<void *> CHUNK_LIST[SIZE_NUM];
thread_local static std::map<void *, size_t> PTR_SIZE_MAP;
static constexpr size_t SLOT_SIZE_MAP[SIZE_NUM] = {
    1UL,      1UL << 1, 1UL << 2, 1UL << 3, 1UL << 4, 1UL << 5,
    1UL << 6, 1UL << 7, 1UL << 8, 1UL << 9, 1UL << 10};

static constexpr size_t MAX_CHACH_CHUNK_SIZE = SLOT_SIZE_MAP[SIZE_NUM - 1];

// If we have C++17, we can use constexpr lambda to check value without annoying
// type traits structs.
static_assert(is_all_positive<const size_t[], SIZE_NUM>::check(SLOT_SIZE_MAP),
              "All number in SLOT_SIZE_MAP must have positive value!");

FrontEnd::FrontEnd() {
  // Do not refill chunks during allocator initialization.
  // Caches are thread local.
  back_end = new AllocatorBase();
}

void FrontEnd::refill(int slot_id) {
  for (int i = 0; i < CHUNK_NUM; ++i) {
    CHUNK_LIST[slot_id].push_back(back_end->alloc(SLOT_SIZE_MAP[slot_id]));
  }
}

void *FrontEnd::alloc(size_t size) {
  size_t round_size = round2pow(size);
  if (round_size > MAX_CHACH_CHUNK_SIZE) {
    return back_end->alloc(round_size);
  }
  size_t slot_id = ctz(round_size);
  if (CHUNK_LIST[slot_id].empty()) {
    refill(slot_id);
  }
  void *ptr = CHUNK_LIST[slot_id].front();
  CHUNK_LIST[slot_id].pop_front();
  PTR_SIZE_MAP[ptr] = round_size;
  return ptr;
}

void FrontEnd::dealloc(void *ptr) {
  auto it = PTR_SIZE_MAP.find(ptr);
  if (it == PTR_SIZE_MAP.end()) {
    back_end->dealloc(ptr);
    return;
  }
  size_t slot_id = ctz(it->second);
  PTR_SIZE_MAP.erase(it);
  CHUNK_LIST[slot_id].push_back(ptr);
}

}  // namespace Detail
}  // namespace UAllocator