#include "memory.h"

void *memset(void *dest, int c, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    uint8_t byte = (uint8_t)c;

    if (n >= 4) {
        uint32_t dword = (uint32_t)byte | ((uint32_t)byte << 8) | ((uint32_t)byte << 16) | ((uint32_t)byte << 24);
        size_t dwords = n >> 2;
        size_t bytes = n & 3;

        __asm__ volatile(
            "cld\n\t"
            "rep stosl\n\t"
            : "+D"(d), "+c"(dwords)
            : "a"(dword)
            : "memory"
        );

        n = bytes;
    }

    if (n > 0) {
        __asm__ volatile(
            "cld\n\t"
            "rep stosb\n\t"
            : "+D"(d), "+c"(n)
            : "a"(byte)
            : "memory"
        );
    }

    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    size_t dwords = n >> 2;
    size_t bytes = n & 3;

    if (dwords > 0) {
        __asm__ volatile(
            "cld\n\t"
            "rep movsl\n\t"
            : "+D"(d), "+S"(s), "+c"(dwords)
            :
            : "memory"
        );
    }

    if (bytes > 0) {
        __asm__ volatile(
            "cld\n\t"
            "rep movsb\n\t"
            : "+D"(d), "+S"(s), "+c"(bytes)
            :
            : "memory"
        );
    }

    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        return memcpy(dest, src, n);
    } else {
        d += n;
        s += n;
        while (n--) {
            *(--d) = *(--s);
        }
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
    }
    return 0;
}
