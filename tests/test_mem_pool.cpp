#include <stdint.h>

#include <iostream>
#include <random>

#include "../src/allocator.h"

using namespace UAllocator::Detail;

int test_fixed_size_pool_single_size(size_t block_size, size_t page_num,
                                     size_t repeate = size_t(3e5)) {
  using Page = FixedBlockSizeMemPool::Page;
  FixedBlockSizeMemPool *pool =
      FixedBlockSizeMemPool::create(block_size, page_num);
  int prevent_opt = ~0;
  for (size_t rd = 0; rd < repeate; ++rd) {
    char *ptr = (char *)pool->allocate();
    for (size_t b = 0; b < block_size; ++b) {
      ptr[b] = 'a' + (b % 26);
    }
    for (size_t b = 0; b < block_size; ++b) {
      if (ptr[b] != 'a' + (b % 26)) {
        return -1;
      }
      prevent_opt &= ptr[b] - 'a' + (b % 26);
    }
    Page *page = reinterpret_cast<Page *>(
        reinterpret_cast<size_t>(ptr) & ~(FixedBlockSizeMemPool::PageSize - 1));
    page->deallocate_block(ptr);
  }
  return prevent_opt;  // is always 0 if correct.
}

int test_fixed_size_pool() {
  size_t block_size[] = {8, 16, 32, 64, 128, 256, 512};
  size_t page_num[] = {1, 2, 3, 5, 8, 10};
  int result = 0;
  for (size_t bs = 0; bs < sizeof(block_size) / sizeof(size_t); ++bs) {
    for (size_t pn = 0; pn < sizeof(page_num) / sizeof(size_t); ++pn) {
      result |= test_fixed_size_pool_single_size(block_size[bs], page_num[pn]);
    }
  }
  return result;
}

int main() { return 0 || test_fixed_size_pool(); }