#ifndef _STRING_H_
#define _STRING_H_

#include "types.h"

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *str);
char *append_signed(char *out, int64_t value);
char *append_unsigned(char *out, uint64_t value, int base);
char *append_string(char *out, const char *str);
char *append_char(char *out, char c);

#endif
