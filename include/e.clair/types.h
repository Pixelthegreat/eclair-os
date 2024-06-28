#ifndef ECLAIR_TYPES_H
#define ECLAIR_TYPES_H

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

typedef long size_t;

#define NULL ((void *)0)

#define ALIGN(x, sz) (((x) + ((sz)-1)) & ~(sz-1))

#endif /* ECLAIR_TYPES_H */
