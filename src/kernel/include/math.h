//
// Created by julian on 27.09.26.
//

#ifndef TEST_OS_MATH_H
#define TEST_OS_MATH_H

#include "types.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define CEIL(x, low, high) (MAX(MIN((x), (high)), (low)))

uint32_t log10_ceil (uint32_t x);

uint64_t __udivdi3(uint64_t a, uint64_t b);
uint64_t __umoddi3(uint64_t a, uint64_t b);
int64_t __divdi3(int64_t a, int64_t b);
int64_t __moddi3(int64_t a, int64_t b);

#endif //TEST_OS_MATH_H
