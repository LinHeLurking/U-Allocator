#include <stddef.h>
#include <stdint.h>

#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <type_traits>

#include "../src/multi_level_radix_tree.h"

int perf_radix_tree(int64_t repeat = (int)3e6) {
  std::map<void *, size_t> target;
  RadixTree::L4RadixTree<void *, size_t> current;
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::uniform_int_distribution<int64_t> dist(1, 1ULL << 30);

  auto map_start = std::chrono::high_resolution_clock::now();
  for (int64_t i = 0; i < repeat; ++i) {
    void *ptr = (void *)dist(gen);
    size_t size = dist(gen);
    target[ptr] = size;
  }
  auto map_end = std::chrono::high_resolution_clock::now();

  auto radix_tree_start = std::chrono::high_resolution_clock::now();
  for (int64_t i = 0; i < repeat; ++i) {
    void *ptr = (void *)dist(gen);
    size_t size = dist(gen);
    current.put(ptr, size);
  }
  auto radix_tree_end = std::chrono::high_resolution_clock::now();

  auto map_perf = double((map_end - map_start).count()) / repeat;
  auto radix_tree_perf =
      double((radix_tree_end - radix_tree_start).count()) / repeat;

  fprintf(stdout, "map perf: %0.6lf ns/op\n", map_perf);
  fprintf(stdout, "radix tree perf: %0.6lf ns/op\n", radix_tree_perf);

  return 0;
}

int main() { return 0 || perf_radix_tree(); }