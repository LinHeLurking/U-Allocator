#ifndef UALLOCATOR_ALLOCATOR_H
#define UALLOCATOR_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

#include <bitset>
#include <list>
#include <map>
#include <vector>

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
  ~AllocatorBase() = default;
  static void *allocate(size_t size) { return malloc(size); }
  static void deallocate(void *ptr) { free(ptr); }
};

class FixedBlockSizeMemPool;

/**
 * @brief Fix-sized page which holds a number of memory blocks.
 * Blocks in the same page have the same size. A [MemPage] must
 * be [Size] aligned. A [MemPage] should not constructed directly
 * because we might want to make many continuous pages.
 */
template <size_t Size = 4096>
class MemPage {
 public:
  struct ListNode {
    ListNode *next_;
  };

  /**
   * @brief Metadata of this page. Block size is not
   * stored in metadata to decrease memory footprint.
   */
  struct Meta {
    FixedBlockSizeMemPool *pool_base_;
    ListNode *plist_free_;
  };

  Meta meta_;

  // Data array holds the real block data.
  char data_[Size - sizeof(Meta)];

  MemPage() = delete;
  ~MemPage() = delete;

  /**
   * @brief Initialize all needed fiels in this page.
   * @param block_size Size of a block in byte. It should be at least
   * sizeof(ListNode).
   * @param pool_base Pointer to the whole pool.
   */
  inline void init(size_t block_size,
                   FixedBlockSizeMemPool *pool_base) noexcept {
    meta_.pool_base_ = pool_base;
    size_t block_num = (Size - sizeof(Meta)) / block_size;

    // Apending all blocks to the free list.
    meta_.plist_free_ = reinterpret_cast<ListNode *>(data_);
    ListNode *cur = meta_.plist_free_;
    for (size_t i = 1; i < block_num; ++i, cur = cur->next_) {
      cur->next_ = reinterpret_cast<ListNode *>(&data_[i * block_size]);
    }
    cur->next_ = nullptr;
  }

  /**
   * @brief Allocate a block from this page.
   * If there's no available data in page, a nullptr is returned.
   */
  inline void *allocate_block() noexcept {
    ListNode *cur = meta_.plist_free_;
    if (cur != nullptr) {
      meta_.plist_free_ = cur->next_;
    }
    return cur;
  }

  /**
   * @brief Put the ptr back to page.
   * It's the caller's duty to guarentee the ptr is allocated from this page.
   */
  inline void deallocate_block(void *ptr) noexcept {
    ListNode *node = reinterpret_cast<ListNode *>(ptr);
    node->next_ = meta_.plist_free_;
    meta_.plist_free_ = node;
  }
};

/**
 * @brief A memory pool contains serveral pages with the same page size.
 * All blocks in the same pool also have the same block size.
 * Due to alignment issues, do not construct [MemPool] directly.
 * Instead, use the [create] method.
 */
class FixedBlockSizeMemPool {
 public:
  static constexpr size_t PageSize = 4096;
  using Page = MemPage<PageSize>;

  struct Meta {
    bool owned;
    size_t block_size_;
    size_t page_num_;
    Page *page_base_;
  };

  static_assert(sizeof(Meta) <= PageSize,
                "Metadata size must not exceed 4K!!!");

  Meta meta_;

  /**
   * @brief Create a [MemPool] instance from given parameters.
   * @param block_size Byte size of blocks in page in pool. Block size should be at least 
   * the size of a [ListNode].
   * @param page_num Number of pages in this pool.
   * @param pool_base If provided, the create method will not malloc data by
   * iteself but use the [pool_base] address. It's the caller's response to
   * guarantee there's enough space pointed by the pointer. This parameter
   * should be provided with [page_base] at the same time.
   * @param page_base Similar as previous one, but points to the base address of
   * first page in pool.
   */
  static inline FixedBlockSizeMemPool *create(
      size_t block_size, size_t page_num, void *pool_base = nullptr,
      void *page_base = nullptr) noexcept {
    if (pool_base == nullptr && page_base == nullptr) {
      size_t size = (page_num + 1) * PageSize +
                    sizeof(Meta);  // extra size to ensure alignment
      size_t self_ptr_val = reinterpret_cast<size_t>(malloc(size));
      size_t page_base_ptr_val = self_ptr_val;
      size_t mask = PageSize - 1;
      if ((page_base_ptr_val & mask) == 0) {
        page_base_ptr_val += PageSize;
      } else {
        page_base_ptr_val &= ~mask;
        page_base_ptr_val += PageSize;
        if (page_base_ptr_val - self_ptr_val < sizeof(Meta)) {
          page_base_ptr_val += PageSize;
        }
      }
      FixedBlockSizeMemPool *self =
          reinterpret_cast<FixedBlockSizeMemPool *>(self_ptr_val);
      self->init(block_size, page_base_ptr_val, page_num);
      self->meta_.owned = true;
      return self;
    } else {
      FixedBlockSizeMemPool *self =
          reinterpret_cast<FixedBlockSizeMemPool *>(pool_base);
      self->init(block_size, reinterpret_cast<size_t>(page_base), page_num);
      self->meta_.owned = false;
      return self;
    }
  }

  inline void *allocate() noexcept {
    for (size_t i = 0; i < meta_.page_num_; ++i) {
      void *ptr = meta_.page_base_[i].allocate_block();
      if (ptr != nullptr) {
        return ptr;
      }
    }
    // If all pages are full, malloc if by libc.
    return malloc(meta_.block_size_);
  }

  FixedBlockSizeMemPool() = delete;
  ~FixedBlockSizeMemPool() {
    if (meta_.owned) {
      free(static_cast<void *>(this));
    }
  }

 protected:
  inline void init(size_t block_size, size_t page_base,
                   size_t page_num) noexcept {
    meta_.page_num_ = page_num;
    meta_.block_size_ = block_size;
    meta_.page_base_ = reinterpret_cast<Page *>(page_base);
    for (size_t i = 0; i < page_num; ++i) {
      meta_.page_base_[i].init(block_size, this);
    }
  }
};

template <typename BackEndAllocator = AllocatorBase>
class FrontEnd {
 protected:
 public:
  FrontEnd() {}

  void *allocate(size_t size) { return nullptr; }
  void deallocate(void *ptr) {}
};
}  // namespace Detail

using Allocator = Detail::FrontEnd<>;
}  // namespace UAllocator
#endif