/* stand-in for now */
#ifndef STDINT_H
#define STDINT_H

/* 8 bit */
typedef char int8_t;
typedef unsigned char uint8_t;

/* 16 bit */
typedef short int16_t;
typedef unsigned short uint16_t;

/* 32 bit */
typedef int int32_t;
typedef unsigned int uint32_t;

/* 64 bit */
#ifdef ECLAIR64
typedef long int64_t;
typedef unsigned long uint64_t;
#endif

#endif /* STDINT_H */
