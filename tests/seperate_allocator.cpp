#include <future>
#include <random>
#include <thread>
#include <vector>

#include "../allocator.h"

int thrd_task_separate(int repeat) {
  auto allocator = UAllocator::Allocator();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 4096);

  std::vector<void *> allocated;

  for (int i = 0; i < repeat; ++i) {
    int coin = dis(gen) % 2;
    if (coin == 1 && !allocated.empty()) {
      void *cur = allocated.back();
      allocated.pop_back();
      allocator.dealloc(cur);
    } else if (coin == 2) {
      int len = dis(gen);
      char *cur = (char *)allocator.alloc(len);
      for (int i = 0; i < len; ++i) {
        cur[i] = 'a';
      }
      allocated.push_back(cur);
    }
  }
  for (void *ptr : allocated) {
    allocator.dealloc(ptr);
  }
  return 0;
}

int test_allocator_seperate() {
  std::vector<std::future<int>> thrds(4);
#ifndef NDEBUG
  constexpr int repeat = int(3e7);
#else
  constexpr int repeat = int(2e8);
#endif
  for (auto &thrd : thrds) {
    thrd = std::async(thrd_task_separate, repeat);
  }
  for (auto &thrd : thrds) {
    if (thrd.get() != 0) {
      return -1;
    }
  }
  return 0;
}

int main() { return 0 || test_allocator_seperate(); }