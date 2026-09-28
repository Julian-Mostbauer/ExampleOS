#include "string.h"

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0') {
            return (const unsigned char)s1[i] - (const unsigned char)s2[i];
        }
    }
    return 0;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}


char *append_char(char *out, char c)
{
    *out++ = c;
    return out;
}

char *append_string(char *out, const char *str)
{
    while (*str)
        *out++ = *str++;

    return out;
}

char *append_unsigned(char *out, uint64_t value, const int base)
{
    char buffer[32];
    int i = 0;

    if (value == 0)
        return append_char(out, '0');

    while (value > 0) {
        const unsigned digit = value % base;

        if (digit < 10)
            buffer[i++] = '0' + digit;
        else
            buffer[i++] = 'a' + (digit - 10);

        value /= base;
    }

    while (i > 0)
        *out++ = buffer[--i];

    return out;
}

char *append_signed(char *out, const int64_t value)
{
    if (value < 0) {
        *out++ = '-';

        /*
         * Avoid -INT64_MIN overflowing.
         */
        return append_unsigned(out, (uint64_t)(-(uint64_t)value), 10);
    }

    return append_unsigned(out, (uint64_t)value, 10);
}