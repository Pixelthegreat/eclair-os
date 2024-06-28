#ifndef STRING_H
#define STRING_H

#include <e.clair/types.h> /* size_t */

extern size_t strlen(const char *str); /* get length of string */

extern void *memset(void *dst, int ch, size_t cnt); /* fill a buffer with the specified character */

#endif /* STRING_H */
