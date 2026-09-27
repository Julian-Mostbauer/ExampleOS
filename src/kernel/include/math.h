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

#endif //TEST_OS_MATH_H
