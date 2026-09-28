#include "math.h"

uint32_t log10_ceil(uint32_t x) {
    uint32_t res = 1;
    while (x > 10) {
        res++;
        x /= 10;
    }
    return res;
}

static uint64_t udivmod64(uint64_t num, uint64_t den, uint64_t *rem_p) {
    if (den == 0) {
        if (rem_p) *rem_p = 0;
        return 0;
    }
    if (num < den) {
        if (rem_p) *rem_p = num;
        return 0;
    }
    if (den == 1) {
        if (rem_p) *rem_p = 0;
        return num;
    }

    uint64_t quot = 0;
    uint64_t rem = 0;

    for (int i = 63; i >= 0; i--) {
        rem = (rem << 1) | ((num >> i) & 1ULL);
        if (rem >= den) {
            rem -= den;
            quot |= (1ULL << i);
        }
    }

    if (rem_p) {
        *rem_p = rem;
    }
    return quot;
}

uint64_t __udivdi3(uint64_t a, uint64_t b) {
    return udivmod64(a, b, NULL);
}

uint64_t __umoddi3(uint64_t a, uint64_t b) {
    uint64_t rem = 0;
    udivmod64(a, b, &rem);
    return rem;
}

int64_t __divdi3(int64_t a, int64_t b) {
    int neg = 0;
    uint64_t ua, ub;
    if (a < 0) {
        ua = (uint64_t)(-a);
        neg = !neg;
    } else {
        ua = (uint64_t)a;
    }
    if (b < 0) {
        ub = (uint64_t)(-b);
        neg = !neg;
    } else {
        ub = (uint64_t)b;
    }
    uint64_t res = udivmod64(ua, ub, NULL);
    return neg ? -(int64_t)res : (int64_t)res;
}

int64_t __moddi3(int64_t a, int64_t b) {
    int neg = 0;
    uint64_t ua, ub;
    if (a < 0) {
        ua = (uint64_t)(-a);
        neg = 1;
    } else {
        ua = (uint64_t)a;
    }
    if (b < 0) {
        ub = (uint64_t)(-b);
    } else {
        ub = (uint64_t)b;
    }
    uint64_t rem = 0;
    udivmod64(ua, ub, &rem);
    return neg ? -(int64_t)rem : (int64_t)rem;
}

