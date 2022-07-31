#include <stdint.h>

#include <iostream>
#include <random>

#include "../src/allocator.h"

using namespace UAllocator::Detail;

int test_fixed_size_pool_single_size(size_t block_size, size_t page_num,
                                     size_t repeate = size_t(1e5),
                                     size_t batch_size = size_t(1e3)) {
  using Page = FixedBlockSizeMemPool::Page;
  FixedBlockSizeMemPool *pool =
      FixedBlockSizeMemPool::create(block_size, page_num);
  int prevent_opt = ~0;
  size_t batch;
  for (size_t rd = 0; rd < repeate;) {
    std::vector<void *> allocated;
    for (batch = 0; batch < batch_size && rd + batch < repeate; ++batch, ++rd) {
      char *ptr = (char *)pool->allocate();
      if (ptr == nullptr) {
        fprintf(stderr, "Allocation failed.\n");
        return -1;
      }
      for (size_t b = 0; b < block_size; ++b) {
        ptr[b] = 'a' + (b % 26);
      }
      for (auto old : allocated) {
        if (old == ptr) {
          fprintf(stderr, "Allocated ptr has the same address.\n");
          return -1;
        }
      }
      allocated.push_back((void *)ptr);
    }
    for (size_t bt = 0; bt < batch; ++bt) {
      char *ptr = (char *)allocated.back();
      allocated.pop_back();
      for (size_t b = 0; b < block_size; ++b) {
        if (ptr[b] != 'a' + (b % 26)) {
          fprintf(stderr,
                  "Block size: %lu, page num: %lu, round %lu, byte %lu, "
                  "expected %c but "
                  "get %c.\n",
                  block_size, page_num, rd - 1 - bt, b, char('a' + (b % 26)),
                  ptr[b]);
          return -1;
        }
        prevent_opt &= ptr[b] - char('a' + (b % 26));
      }
      pool->deallocate((void *)ptr);
    }
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