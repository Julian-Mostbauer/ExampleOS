#include "math.h"

uint32_t log10_ceil(uint32_t x) {
    uint32_t res = 1;
    while (x > 10) {
        res++;
        x /= 10;
    }
    return res;
}
