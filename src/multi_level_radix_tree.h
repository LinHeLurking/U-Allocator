#ifndef UALLOCATOR_MULTI_LEVEL_RADIX_TREE_H
#define UALLOCATOR_MULTI_LEVEL_RADIX_TREE_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <bitset>
#include <type_traits>

namespace RadixTree {

namespace Detail {

struct NaiveAllocator {
  inline void *allocate(size_t size) { return malloc(size); }
  inline void deallocate(void *ptr) { free(ptr); }
};

namespace {
template <typename T, typename = bool>
struct FitUInt {};

template <typename T>
struct FitUInt<T, typename std::enable_if<sizeof(T) == 4, bool>::type> {
  using type = uint32_t;
};

template <typename T>
struct FitUInt<T, typename std::enable_if<sizeof(T) == 8, bool>::type> {
  using type = uint64_t;
};

template <typename T>
inline typename FitUInt<T>::type cast_uint(T x) {
  return reinterpret_cast<typename FitUInt<T>::type>(x);
}
}  // namespace

template <typename K, typename V, size_t KeySegBitWidth, size_t ChunkBitWidth,
          typename Allocator>
class L4ChunkyRadixTree {
 protected:
  struct LeafNode {
    V data[1ULL << ChunkBitWidth] = {};
    std::bitset<(1ULL << ChunkBitWidth)> occ_map{0};
  };

  struct TreeNode {
    void *child[1ULL << KeySegBitWidth];
  };

  void *child[1ULL << KeySegBitWidth] = {};

  Allocator *allocator = new Allocator();

 public:
  static constexpr size_t SEG_MASK = (1ULL << KeySegBitWidth) - 1;
  static constexpr size_t CHUNK_MASK = (1ULL << ChunkBitWidth) - 1;

  inline V *get(K key) noexcept {
    auto _key = cast_uint(key);
    auto seg_1 = (_key >> (3 * KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_2 = (_key >> (2 * KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_3 = (_key >> (KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_4 = (_key >> ChunkBitWidth) & SEG_MASK;
    auto seg_5 = _key & CHUNK_MASK;

    void **ptr = child;
    // First jump
    if (ptr[seg_1] == nullptr) {
      return nullptr;
    }
    ptr = (void **)ptr[seg_1];
    // Second jump
    if (ptr[seg_2] == nullptr) {
      return nullptr;
    }
    ptr = (void **)ptr[seg_2];
    // Third jump
    if (ptr[seg_3] == nullptr) {
      return nullptr;
    }
    ptr = (void **)ptr[seg_3];
    // Fourth jump
    if (ptr[seg_4] == nullptr) {
      return nullptr;
    }
    ptr = (void **)ptr[seg_4];
    // Now ptr points to data leaf, which is a (1ULL << ChunkBitWidth) sized
    // chunk.
    LeafNode *leaf = reinterpret_cast<LeafNode *>(ptr);
    if (leaf->occ_map[seg_5]) {
      return &(leaf->data[seg_5]);
    } else {
      return nullptr;
    }
  }

  inline void put(K key, V value) {
    auto _key = cast_uint(key);
    auto seg_1 = (_key >> (3 * KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_2 = (_key >> (2 * KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_3 = (_key >> (KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_4 = (_key >> ChunkBitWidth) & SEG_MASK;
    auto seg_5 = _key & CHUNK_MASK;

    void **ptr = child;
    // First jump
    if (ptr[seg_1] == nullptr) {
      ptr[seg_1] = allocator->allocate(sizeof(TreeNode));
    }
    ptr = (void **)ptr[seg_1];
    // Second jump
    if (ptr[seg_2] == nullptr) {
      ptr[seg_2] = allocator->allocate(sizeof(TreeNode));
    }
    ptr = (void **)ptr[seg_2];
    // Third jump
    if (ptr[seg_3] == nullptr) {
      ptr[seg_3] = allocator->allocate(sizeof(TreeNode));
    }
    ptr = (void **)ptr[seg_3];
    // Fourth jump
    if (ptr[seg_4] == nullptr) {
      ptr[seg_4] = allocator->allocate(sizeof(LeafNode));
    }
    ptr = (void **)ptr[seg_4];
    // Now ptr points to data leaf, which is a (1ULL << ChunkBitWidth) sized
    // chunk.
    LeafNode *leaf = reinterpret_cast<LeafNode *>(ptr);
    leaf->data[seg_5] = value;
    leaf->occ_map[seg_5] = 1;
  }

  inline void remove(K key) noexcept {
    auto _key = cast_uint(key);
    auto seg_1 = (_key >> (3 * KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_2 = (_key >> (2 * KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_3 = (_key >> (KeySegBitWidth + ChunkBitWidth)) & SEG_MASK;
    auto seg_4 = (_key >> ChunkBitWidth) & SEG_MASK;
    auto seg_5 = _key & CHUNK_MASK;

    void **ptr = child;
    // First jump
    if (ptr[seg_1] == nullptr) {
      return;
    }
    ptr = (void **)ptr[seg_1];
    // Second jump
    if (ptr[seg_2] == nullptr) {
      return;
    }
    ptr = (void **)ptr[seg_2];
    // Third jump
    if (ptr[seg_3] == nullptr) {
      return;
    }
    ptr = (void **)ptr[seg_3];
    // Fourth jump
    if (ptr[seg_4] == nullptr) {
      return;
    }
    ptr = (void **)ptr[seg_4];
    // Now ptr points to data leaf, which is a (1ULL << ChunkBitWidth) sized
    // chunk.
    LeafNode *leaf = reinterpret_cast<LeafNode *>(ptr);
    leaf->occ_map &= ~(1 << seg_5);
  }
};
}  // namespace Detail

// 64-bit addressing
template <typename K, typename V, typename Allocator = Detail::NaiveAllocator>
using L4RadixTree = Detail::L4ChunkyRadixTree<K, V, 16, 0, Allocator>;

}  // namespace RadixTree

#endif