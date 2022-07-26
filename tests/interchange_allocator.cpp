#include <future>
#include <random>
#include <thread>
#include <vector>

#include "../src/allocator.h"
#include "mem_wr.h"

int thrd_task_interchange(int tid, int repeat, std::vector<std::mutex> &mutexes,
                          std::vector<std::vector<void *>> &allocated) {
  auto allocator = UAllocator::Allocator();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 4096);

  for (int i = 0; i < repeat; ++i) {
    int coin = dis(gen) % 2;
    if (coin == 1) {
      mutexes[tid].lock();
      if (allocated[tid].empty()) {
        mutexes[tid].unlock();
        continue;
      }
      void *cur = allocated[tid].back();
      allocated[tid].pop_back();
      allocator.dealloc(cur);
      mutexes[tid].unlock();
    } else if (coin == 2) {
      int len = dis(gen);
      mutexes[tid].lock();
      char *cur = (char *)allocator.alloc(len);
      if (mem_wr(cur, len) != 0) {
        return -1;
      }
      allocated[tid].push_back(cur);
      mutexes[tid].unlock();
    }
  }

  return 0;
}

int test_allocator_interchange() {
  std::vector<std::future<int>> thrds(4);
  std::vector<std::mutex> mutexes(4);
  std::vector<std::vector<void *>> allocated(4);
#ifndef NDEBUG
  constexpr int repeat = int(1e8);
#else
  constexpr int repeat = int(1e8);
#endif
  for (int i = 0; i < thrds.size(); ++i) {
    thrds[i] = std::async(thrd_task_interchange, i, repeat, std::ref(mutexes),
                          std::ref(allocated));
  }
  for (auto &thrd : thrds) {
    if (thrd.get() != 0) {
      return -1;
    }
  }
  return 0;
}

int main() { return 0 || test_allocator_interchange(); }