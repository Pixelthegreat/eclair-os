#ifndef ECLAIR_TYPES_H
#define ECLAIR_TYPES_H

/* basic integer types */
typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

#ifdef ECLAIR64
typedef long int64_t;
typedef unsigned long uint64_t;
#endif

/* stddef stuff */
typedef long size_t;

#define NULL ((void *)0)

/* boolean */
typedef uint8_t bool;

#define true ((bool)1)
#define false ((bool)0)

/* redundant */
#define IS_BOOLEAN_TRUE(b) ((b) == true)
#define IS_BOOLEAN_FALSE(b) ((b) == false)

/* doesn't work */
#define IS_REDUNDANT(m) ((m) == IS_BOOLEAN_TRUE || (m) == IS_BOOLEAN_FALSE)

/* misc */
#define ALIGN(x, sz) (((x) + ((sz)-1)) & ~(sz-1))

#endif /* ECLAIR_TYPES_H */
