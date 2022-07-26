#ifndef UALLOCATOR_MEM_WR_H
#define UALLOCATOR_MEM_WR_H

#include <stddef.h>

int mem_wr(char *ptr, size_t len) {
  for (int rd = 0; rd < 10000; ++rd) {
    for (int i = 0; i < len; ++i) {
      ptr[i] = 'a' + (i % 26);
      if (ptr[i] != 'a' + (i % 26)) {
        return -1;
      }
    }
    for (size_t i = 0; i < len; ++i) {
      ptr[i] = 'a' + (i % 26);
    }
    for (size_t i = 0; i < len; ++i) {
      if (ptr[i] != 'a' + (i % 26)) {
        return -1;
      }
    }
  }
  return 0;
}

#endif  // UALLOCATOR_MEM_WR_H