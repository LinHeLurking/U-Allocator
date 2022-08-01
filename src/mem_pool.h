#ifndef UALLOCATOR_MEM_POOL_H
#define UALLOCATOR_MEM_POOL_H

#include <stddef.h>
#include <stdlib.h>

#include <algorithm>

namespace UAllocator {
namespace Detail {

template <typename Int32,
          typename std::enable_if<std::is_integral<Int32>::value &&
                                      sizeof(Int32) == 4,
                                  bool>::type = true>
Int32 round2pow(Int32 x) {
  return x == 0 ? 1 : ((x & (x - 1)) == 0) ? x : 1 << (32 - __builtin_clz(x));
}

template <typename Int64,
          typename std::enable_if<std::is_integral<Int64>::value &&
                                      sizeof(Int64) == 8,
                                  bool>::type = true>
Int64 round2pow(Int64 x) {
  return x == 0                 ? 1
         : ((x & (x - 1)) == 0) ? x
                                : 1UL << (64 - __builtin_clzll(x));
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

template <size_t PageSize>
class FixedBlockSizeMemPool;

/**
 * @brief Fix-sized page which holds a number of memory blocks.
 * Blocks in the same page have the same size. A [MemPage] must
 * be [Size] aligned. A [MemPage] should not constructed directly
 * because we might want to make many continuous pages.
 */
template <size_t PageSize>
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
    FixedBlockSizeMemPool<PageSize> *pool_base_;
    ListNode *plist_free_;
  };

  Meta meta_;

  // Data array holds the real block data.
  char data_[PageSize - sizeof(Meta)];

  MemPage() = delete;
  ~MemPage() = delete;

  /**
   * @brief Initialize all needed fields in this page.
   * @param block_size Size of a block in byte. It should be at least
   * sizeof(ListNode).
   * @param pool_base Pointer to the whole pool.
   */
  inline void init(size_t block_size,
                   FixedBlockSizeMemPool<PageSize> *pool_base) noexcept {
    meta_.pool_base_ = pool_base;
    size_t block_num = (PageSize - sizeof(Meta)) / block_size;

    // Appending all blocks to the free list.
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
   * It's the caller's duty to guarantee the ptr is allocated from this page.
   */
  inline void deallocate_block(void *ptr) noexcept {
#ifndef NDEBUG
    // Check if the ptr is from this page
    if (ptr < this || ptr >= this + Size) {
      fprintf(stderr, "Error: deallocate an external pointer to this page!\n");
    }
#endif
    ListNode *node = reinterpret_cast<ListNode *>(ptr);
    node->next_ = meta_.plist_free_;
    meta_.plist_free_ = node;
  }
};

/**
 * @brief A memory pool contains several pages with the same page size.
 * All blocks in the same pool also have the same block size.
 * Due to alignment issues, do not construct [MemPool] directly.
 * Instead, use the [create] method.
 */
template <size_t PageSize>
class FixedBlockSizeMemPool {
 public:
  using Page = MemPage<PageSize>;

  struct Meta {
    bool owned;
    size_t block_size_;
    size_t page_num_;
    Page *page_base_;
    Page *page_end_;
  };

  static_assert(sizeof(Meta) <= PageSize,
                "Metadata size must not exceed 4K!!!");

  Meta meta_;

  /**
   * @brief Create a [MemPool] instance from given parameters.
   * @param block_size Byte size of blocks in page in pool. Block size should be
   * at least the size of a [ListNode].
   * @param page_num Number of pages in this pool.
   * @param pool_base If provided, the create method will not malloc data by
   * itself but use the [pool_base] address. It's the caller's response to
   * guarantee there's enough space pointed by the pointer. This parameter
   * should be provided with [page_base] at the same time.
   * @param page_base Similar as previous one, but points to the base address of
   * first page in pool.
   */
  static inline FixedBlockSizeMemPool<PageSize> *create(
      size_t block_size, size_t page_num, void *pool_base = nullptr,
      void *page_base = nullptr) noexcept {
    if (pool_base == nullptr && page_base == nullptr) {
      // Aligned new is C++17 feature. Now we have to manually translate
      // addresses.
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
      FixedBlockSizeMemPool<PageSize> *self =
          reinterpret_cast<FixedBlockSizeMemPool<PageSize> *>(self_ptr_val);
      self->init(block_size, page_base_ptr_val, page_num);
      self->meta_.owned = true;
      return self;
    } else {
      FixedBlockSizeMemPool<PageSize> *self =
          reinterpret_cast<FixedBlockSizeMemPool<PageSize> *>(pool_base);
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

  /**
   * @brief Give a pointer back to the pool.
   * It's the caller's duty to guarantee the ptr is allocated from this pool.
   */
  inline void deallocate(void *ptr) noexcept {
    // If everything is right, it never reaches this branch.
    // Because the outer level pool will do necessary checks.
    // But we add this for more safety. This behavior will be tested in CTEST.
    if (ptr < meta_.page_base_ || ptr >= meta_.page_end_) {
      free(ptr);
      return;
    }
    Page *page = reinterpret_cast<Page *>(reinterpret_cast<size_t>(ptr) &
                                          ~(PageSize - 1));
    page->deallocate_block(ptr);
  }

  /**
   * @brief Give a pointer back to the pool.
   * It's the caller's duty to guarantee the ptr is allocated from this pool.
   * This version has no check about the pointer.
   */
  inline void deallocate_unsafe(void *ptr) noexcept {
    Page *page = reinterpret_cast<Page *>(reinterpret_cast<size_t>(ptr) &
                                          ~(PageSize - 1));
    page->deallocate_block(ptr);
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
    meta_.page_end_ =
        reinterpret_cast<Page *>(page_base + sizeof(Page) * page_num);
    for (size_t i = 0; i < page_num; ++i) {
      meta_.page_base_[i].init(block_size, this);
    }
  }
};

/**
 * @brief A [MemPool] contains several different sized [FixedBlockSizeMemPool],
 * which cache a number of small chunk of memory.
 * On allocation, MemPool will choose a suitable pool and allocate memory
 * from it. On deallocation, MemPool can recognize whether the pointer
 * comes from the MemPool cache or directly from libc malloc and send the
 * data back to MemPool cache or system memory correctly.
 */
template <size_t PageSize = 4096>
class MemPool {
 public:
  // Number of different block sizes;
  static constexpr size_t SizeNum = 8;
  // An array containing the block_size:num_of_page pairs.
  // TODO: Add static check to guarantee SizeDist is valid.
  static constexpr std::pair<size_t, size_t> SizeDist[SizeNum] = {
      {8, 16},  {16, 16}, {32, 16}, {64, 8},
      {128, 8}, {256, 4}, {512, 4}, {1024, 4}};
  // Size threshold about whether the MemPool caches it.
  static constexpr size_t Threshold = SizeDist[SizeNum - 1].first;

  using Page = MemPage<PageSize>;

  struct Meta {
    void *pool_begin_;
    void *pool_end_;
    FixedBlockSizeMemPool<PageSize> *pool[SizeNum];
  };

  static_assert(sizeof(Meta) <= PageSize, "Metadata must fit in a page.");

  Meta meta_;

  MemPool() = delete;
  ~MemPool() { free(static_cast<void *>(this)); }

  static MemPool *create() noexcept {
    // Aligned new is C++17 feature. Now we have to manually translate
    // addresses.
    size_t need_page_num = 0;
    for (size_t i = 0; i < SizeNum; ++i) {
      need_page_num += SizeDist[i].second + 1;
    }
    size_t meta_ptr_val = reinterpret_cast<size_t>(
        malloc((need_page_num + 1) * PageSize + sizeof(Meta)));
    size_t pool_ptr_val = meta_ptr_val;
    if ((meta_ptr_val & (PageSize - 1)) == 0) {
      // If it's already page aligned.
      pool_ptr_val += PageSize;
    } else {
      pool_ptr_val &= ~(PageSize - 1);
      pool_ptr_val += PageSize;
      if (pool_ptr_val - meta_ptr_val < sizeof(Meta)) {
        pool_ptr_val += PageSize;
      }
    }
    MemPool *self = reinterpret_cast<MemPool *>(meta_ptr_val);
    self->meta_.pool_begin_ = reinterpret_cast<void *>(pool_ptr_val);
    self->meta_.pool_end_ =
        reinterpret_cast<void *>(pool_ptr_val + need_page_num * PageSize);
    for (size_t cur = pool_ptr_val, i = 0; i < SizeNum;
         cur += (SizeDist[i].second + 1) * PageSize, i += 1) {
      size_t block_size = SizeDist[i].first;
      size_t page_num = SizeDist[i].second;
      void *pool_base = reinterpret_cast<void *>(cur);
      void *page_base = reinterpret_cast<void *>(cur + PageSize);
      self->meta_.pool[i] = FixedBlockSizeMemPool<PageSize>::create(
          block_size, page_num, pool_base, page_base);
    }
    return self;
  }

  inline size_t get_pool_id(size_t round_size) const noexcept {
    // A hack here because the smallest round_size is 8, which is 2**3;
    return ctz(round_size) - 3;
  }

  void *allocate(size_t size) noexcept {
    if (size > Threshold) {
      return malloc(size);
    }
    size_t round_size = std::max(round2pow(size), SizeDist[0].first);
    size_t id = get_pool_id(round_size);
    return meta_.pool[id]->allocate();
  }

  void deallocate(void *ptr) noexcept {
    if (ptr < meta_.pool_begin_ || ptr >= meta_.pool_end_) {
      free(ptr);
      return;
    }
    Page *page = reinterpret_cast<Page *>(reinterpret_cast<size_t>(ptr) &
                                          ~(PageSize - 1));
    page->deallocate_block(ptr);
  }
};

// In C++11, we have to redeclare them in namespace scope again.
template <size_t PageSize>
constexpr size_t MemPool<PageSize>::SizeNum;
template <size_t PageSize>
constexpr std::pair<size_t, size_t>
    MemPool<PageSize>::SizeDist[MemPool<PageSize>::SizeNum];
template <size_t PageSize>
constexpr size_t MemPool<PageSize>::Threshold;

}  // namespace Detail
}  // namespace UAllocator

#endif