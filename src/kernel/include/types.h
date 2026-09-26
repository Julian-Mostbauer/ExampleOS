#ifndef _TYPES_H_
#define _TYPES_H_

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned int   size_t;

typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;

#if !defined(__bool_true_false_are_defined) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
typedef int            bool;
#define true  1
#define false 0
#endif

#define NULL ((void *)0)

#endif
