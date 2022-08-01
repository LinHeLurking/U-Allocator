#include <stdint.h>

#include <iostream>
#include <random>

#include "../src/allocator.h"
#include "../src/mem_pool.h"

using namespace UAllocator::Detail;

int test_fixed_size_pool_single_size(size_t block_size, size_t page_num,
                                     size_t batch_num = size_t(5e2),
                                     size_t batch_size = size_t(1e3)) {
  using Page = FixedBlockSizeMemPool<4096, 64>::Page;
  FixedBlockSizeMemPool<4096, 64> *pool =
      FixedBlockSizeMemPool<4096, 64>::create(block_size, page_num);
  int prevent_opt = ~0;
  for (size_t rd = 0; rd < batch_num; ++rd) {
    std::vector<void *> allocated;
    for (size_t id = 0; id < batch_size; ++id) {
      char *ptr = (char *)pool->allocate();
      if (ptr == nullptr) {
        fprintf(stderr, "Allocation failed.\n");
        return -1;
      }
      for (size_t b = 0; b < block_size; ++b) {
        ptr[b] = 'a' + (b + size_t(ptr)) % 26;
      }
      for (auto old : allocated) {
        if (old == ptr) {
          fprintf(stderr, "Allocated ptr has the same address.\n");
          return -1;
        }
      }
      allocated.push_back((void *)ptr);
    }
    for (size_t id = 0; id < batch_size; ++id) {
      char *ptr = (char *)allocated.back();
      allocated.pop_back();
      for (size_t b = 0; b < block_size; ++b) {
        if (ptr[b] != 'a' + (b + size_t(ptr)) % 26) {
          fprintf(
              stderr,
              "Block size: %lu, page num: %lu, round %lu, batch %lu, byte %lu, "
              "expected %c but "
              "get %c.\n",
              block_size, page_num, rd, id, b,
              char('a' + (b + size_t(ptr)) % 26), ptr[b]);
          return -1;
        }
        prevent_opt &= ptr[b] - char('a' + (b + size_t(ptr)) % 26);
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

int test_mem_pool(size_t batch_num = size_t(5e2),
                  size_t batch_size = size_t(1e3)) {
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::uniform_int_distribution<> dist(1, 3000);
  auto pool = MemPool<>::create();
  int prevent_opt = ~0;
  for (size_t cur_b = 0; cur_b < batch_num; ++cur_b) {
    std::vector<std::pair<char *, size_t>> allocated;
    for (size_t id = 0; id < batch_size; ++id) {
      size_t size = dist(gen);
      char *ptr = (char *)pool->allocate(size);
      for (size_t b = 0; b < size; ++b) {
        ptr[b] = 'a' + (b + size_t(ptr)) % 26;
      }
      allocated.push_back({ptr, size});
    }
    for (size_t id = 0; id < batch_size; ++id) {
      size_t size = allocated.back().second;
      char *ptr = allocated.back().first;
      allocated.pop_back();
      for (size_t b = 0; b < size; ++b) {
        if (ptr[b] != 'a' + (b + size_t(ptr)) % 26) {
          fprintf(stderr,
                  "Round %lu, batch %lu, byte %lu, expected %c but get %c\n",
                  cur_b, id, b, char('a' + (b + size_t(ptr)) % 26), ptr[b]);
          return -1;
        }
        prevent_opt &= ptr[b] - char('a' + (b + size_t(ptr)) % 26);
      }
    }
  }
  return prevent_opt;  // is always 0 if correct.
}

int main() { return 0 || test_fixed_size_pool() || test_mem_pool(); }