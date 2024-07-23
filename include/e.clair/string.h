#ifndef STRING_H
#define STRING_H

#include <e.clair/types.h> /* size_t */

extern size_t strlen(const char *str); /* get length of string */
extern char *strcpy(char *dst, const char *src); /* copy string */
extern char *strncpy(char *dst, const char *src, size_t cnt); /* copy string with number of bytes */

extern void *memset(void *dst, int ch, size_t cnt); /* fill a buffer with the specified character */

#endif /* STRING_H */
