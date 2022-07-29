#include <stddef.h>
#include <stdint.h>

#include <iostream>
#include <map>
#include <random>
#include <type_traits>

#include "../src/multi_level_radix_tree.h"

#define BATCH_SIZE 25

int test_correctness(int64_t repeat = (int)2e5) {
  std::map<void *, size_t> target;
  RadixTree::L4RadixTree<void *, size_t> current;
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::uniform_int_distribution<int64_t> dist(1, (1ULL << 30));
  for (int64_t i = 0; i < repeat; ++i) {
    void *ptr[BATCH_SIZE];
    size_t size[BATCH_SIZE];
    for (int j = 0; j < BATCH_SIZE; ++j) {
      ptr[j] = (void *)dist(gen);
      size[j] = dist(gen);
      target[ptr[j]] = size[j];
      current.put(ptr[j], size[j]);
    }
    for (int j = 0; j < BATCH_SIZE; ++j) {
      bool target_found = target.find(ptr[j]) != target.end();
      bool current_found = current.get(ptr[j]) != nullptr;
      // Might have the same ptr[]
      if (target_found != current_found) {
        fprintf(stderr,
                "ERROR: round: %ld, j: %d, ptr: %p, found in std::map: %d, "
                "found in radix tree: %d\n",
                i, j, ptr[j], target_found, current_found);
        return -1;
      }
      if (target_found && target[ptr[j]] != *current.get(ptr[j])) {
        fprintf(stderr,
                "ERROR: round: %ld, j: %d, ptr: %p, size: %lu, in std::map: "
                "%lu, in radix tree: "
                "%lu\n",
                i, j, ptr[j], size[j], target[ptr[j]], *current.get(ptr[j]));
        return -1;
      }
    }
  }
  return 0;
}

int main() { return 0 || test_correctness(); }
