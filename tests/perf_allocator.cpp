#include <stdlib.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "../src/allocator.h"

struct NaiveAllocator {
  static void *alloc(size_t size) { return malloc(size); }
  static void dealloc(void *ptr) { free(ptr); }
};

template <typename Allocator>
void thrd_task(Allocator &allocator, int64_t repeat) {
  int64_t step = 1000;
  for (int64_t rd = 0; rd < repeat; rd += 100) {
    std::vector<void *> ptr;
    for (int64_t i = step * rd; i < std::min(repeat, step * rd + step); ++i) {
      ptr.push_back(allocator.alloc(1 + (i & 0xFFFF)));
    }
    for (int64_t i = 0; i < ptr.size(); ++i) {
      allocator.dealloc(ptr[i]);
    }
  }
}

template <typename Allocator>
double perf_one(const char *name, int thrd_num, int64_t repeat) {
  static_assert(std::is_same<void *, decltype(std::declval<Allocator>().alloc(
                                         (size_t)(1)))>::value,
                "Parameter allocator must have alloc method!");
  static_assert(std::is_same<void, decltype(std::declval<Allocator>().dealloc(
                                       (void *)(0)))>::value,
                "Parameter allocator must have dealloc method!");

  auto print_perf = [](const char *name, double perf) {
    fprintf(stdout, "%s: %0.6lf s\n", name, perf);
  };
  auto allocator = Allocator();
  std::vector<std::thread> thrds;
  std::vector<std::mutex> mutexes(4);
  auto clk = std::chrono::high_resolution_clock();
  auto test_start_time = clk.now();
  for (int i = 0; i < thrd_num; ++i) {
    thrds.emplace_back(thrd_task<Allocator>, std::ref(allocator), repeat);
  }
  for (int i = 0; i < thrd_num; ++i) {
    thrds[i].join();
  }
  auto test_end_time = clk.now();
  auto test_duration =
      (test_end_time - test_start_time).count();  // # of nano-seconds.
  double perf_rate = double(test_duration) / 1000 / 1000 / 1000;  // s
  print_perf(name, perf_rate);
  return perf_rate;
}

int perf_all(int thrd_num, int64_t repeat, int epoch = 10) {
  double naive = 0, ua = 0;
  for (int i = 0; i < epoch; ++i) {
    fprintf(stdout, "epoch: %d\n", i);
    naive += perf_one<NaiveAllocator>("Naive allocator", thrd_num, repeat);
    ua += perf_one<UAllocator::Allocator>("U allocator", thrd_num, repeat);
    fprintf(stdout, "\n");
  }
  fprintf(stdout, "Average:\n");
  fprintf(stdout, "Naive allocator: %0.6lf s\n", naive / epoch);
  fprintf(stdout, "U allocator: %0.6lf s\n", ua / epoch);
  return 0;
}

int main() {
  return 0 ||
#ifdef NDEBUG
         perf_all(8, int64_t(3e7))
#else
         perf_all(8, int64_t(3e7))
#endif
      ;
}