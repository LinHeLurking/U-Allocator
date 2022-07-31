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
  inline void *allocate(size_t size) { return malloc(size); }
  inline void deallocate(void *ptr) { free(ptr); }
};

template <typename Allocator>
int64_t thrd_task(Allocator &allocator, int64_t repeat) {
  int64_t prevent_opt = 0;
  for (int64_t i = 0; i < repeat; ++i) {
    // void *ptr = allocator.allocate(1 + (i & 0x3ff));
    void *ptr = allocator.allocate(1 + (i & 0xffff));
    prevent_opt ^= (int64_t)ptr ^ *(char *)ptr;
    allocator.deallocate(ptr);
  }
  return prevent_opt;
}

template <typename Allocator>
int64_t perf_one(const char *name, int thrd_num, int64_t repeat) {
  static_assert(
      std::is_same<void *, decltype(std::declval<Allocator>().allocate(
                               (size_t)(1)))>::value,
      "Parameter allocator must have allocate method!");
  static_assert(
      std::is_same<void, decltype(std::declval<Allocator>().deallocate(
                             (void *)(0)))>::value,
      "Parameter allocator must have deallocate method!");

  auto print_perf = [](const char *name, double perf) {
    fprintf(stdout, "%s: %0.6lf ns/op\n", name, perf);
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
  double perf_rate = double(test_duration) / repeat / 2;
  print_perf(name, perf_rate);
  return test_duration;
}

int perf_all(int thrd_num, int64_t repeat, int epoch = 10) {
#define _perf_one(NAME) perf_one<NAME>(#NAME, thrd_num, repeat)
#define _summary(NAME, cnt)                             \
  {                                                     \
    fprintf(stdout, #NAME " allocator: %0.6lf ns/op\n", \
            ((double)cnt) / epoch / repeat / 2);        \
  }

  int64_t naive_cnt = 0, base_cnt = 0, ua_cnt = 0;
  for (int i = 0; i < epoch; ++i) {
    fprintf(stdout, "epoch: %d\n", i);
    naive_cnt += _perf_one(NaiveAllocator);
    ua_cnt += _perf_one(UAllocator::Allocator);
    fprintf(stdout, "\n");
  }
  fprintf(stdout, "Average:\n");
  _summary(NaiveAllocator, naive_cnt);
  _summary(UAllocator::Allocator, ua_cnt);
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