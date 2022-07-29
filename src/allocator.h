#ifndef UALLOCATOR_ALLOCATOR_H
#define UALLOCATOR_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

#include <bitset>
#include <list>
#include <map>

#include "multi_level_radix_tree.h"

namespace UAllocator {
namespace Detail {

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

class AllocatorBase {
 public:
  AllocatorBase() = default;
  virtual ~AllocatorBase() = default;
  virtual void *allocate(size_t size) { return malloc(size); }
  virtual void deallocate(void *ptr) { free(ptr); }
};

template <size_t ChunkNum, size_t ElemInChunk, size_t ElemSize>
struct CacheLine {
  struct Data {
    char bytes[ElemSize];
  };
  struct Chunk {
    Data data[ElemSize * ElemInChunk];
    std::bitset<ElemInChunk> occupied{0};
  };
  std::list<Chunk> chunk{ChunkNum};

  // Allocate one elem
  inline void *allocate() {
    for (auto it = chunk.begin(); it != chunk.end(); ++it) {
      // Fast skip
      if (it->occupied.count() == ElemInChunk) {
        continue;
      }
      for (size_t b = 0; b < ElemInChunk; ++b) {
        if (!it->occupied[b]) {
          it->occupied[b] = 1;
          return &(it->data[b]);
        }
      }
    }
    for (size_t i = 0; i < ChunkNum; ++i) {
      chunk.emplace_back();
    }
    chunk.back().occupied[0] = 1;
    return &(chunk.back().data[0]);
  }

  inline void deallocate(void *ptr) {
    for (auto it = chunk.begin(); it != chunk.end(); ++it) {
      size_t b = ((Data *)ptr - it->data);
      if (b >= 0 && b < ElemInChunk) {
        it->occupied[b] = 0;
        if (chunk.size() >= 2 * ChunkNum &&
            it->occupied.count() == ElemInChunk) {
          chunk.erase(it);
        }
        return;
      }
    }
  }
};

template <size_t ElemInChunk = 8>
struct CacheManager {
  static constexpr size_t Threshold = 1024;
  CacheLine<32, ElemInChunk, 1> size_1;
  CacheLine<32, ElemInChunk, 2> size_2;
  CacheLine<32, ElemInChunk, 4> size_4;
  CacheLine<32, ElemInChunk, 8> size_8;
  CacheLine<32, ElemInChunk, 16> size_16;
  CacheLine<32, ElemInChunk, 32> size_32;
  CacheLine<16, ElemInChunk, 64> size_64;
  CacheLine<16, ElemInChunk, 128> size_128;
  CacheLine<16, ElemInChunk, 256> size_256;
  CacheLine<16, ElemInChunk, 512> size_512;
  CacheLine<16, ElemInChunk, 1024> size_1024;

  inline void *allocate(size_t round_size) {
    switch (round_size) {
      case 1:
        return size_1.allocate();
      case 2:
        return size_2.allocate();
      case 4:
        return size_4.allocate();
      case 8:
        return size_8.allocate();
      case 16:
        return size_16.allocate();
      case 32:
        return size_32.allocate();
      case 64:
        return size_64.allocate();
      case 128:
        return size_128.allocate();
      case 256:
        return size_256.allocate();
      case 512:
        return size_512.allocate();
      case 1024:
        return size_1024.allocate();
      default:
        return nullptr;
    }
  }

  inline void deallocate(void *ptr, size_t round_size) {
    switch (round_size) {
      case 1:
        size_1.deallocate(ptr);
        break;
      case 2:
        size_2.deallocate(ptr);
        break;
      case 4:
        size_4.deallocate(ptr);
        break;
      case 8:
        size_8.deallocate(ptr);
        break;
      case 16:
        size_16.deallocate(ptr);
        break;
      case 32:
        size_32.deallocate(ptr);
        break;
      case 64:
        size_64.deallocate(ptr);
        break;
      case 128:
        size_128.deallocate(ptr);
        break;
      case 256:
        size_256.deallocate(ptr);
        break;
      case 512:
        size_512.deallocate(ptr);
        break;
      case 1024:
        size_1024.deallocate(ptr);
        break;
    }
  }
};

static thread_local CacheManager<> cache;

template <typename BackEndAllocator = AllocatorBase>
class FrontEnd : public AllocatorBase {
 protected:
  // AllocatorBase *back_end;
  // RadixTree::L4RadixTree<void *, size_t> ptr_size_map;
  std::map<void *, size_t> ptr_size_map;

 public:
  FrontEnd() {}

  void *allocate(size_t size) override {
    if (size > cache.Threshold) {
      return BackEndAllocator::allocate(size);
    }
    size_t round_size = round2pow(size);
    void *ptr = cache.allocate(round_size);
    // ptr_size_map.put(ptr, round_size);
    ptr_size_map[ptr] = round_size;
    return ptr;
  }
  void deallocate(void *ptr) override {
    // size_t *size = ptr_size_map.get(ptr);
    // if (size == nullptr) {
    //   BackEndAllocator::deallocate(ptr);
    //   return;
    // }
    // ptr_size_map.remove(ptr);
    // cache.deallocate(ptr, *size);
    if (ptr_size_map.find(ptr) == ptr_size_map.end()) {
      BackEndAllocator::deallocate(ptr);
      return;
    }
    cache.deallocate(ptr, ptr_size_map[ptr]);
  }
};
}  // namespace Detail

using Allocator = Detail::FrontEnd<>;
}  // namespace UAllocator
#endif