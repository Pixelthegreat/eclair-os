#ifndef STRING_H
#define STRING_H

#include <kernel/types.h> /* size_t */

extern size_t strlen(const char *str); /* get length of string */
extern char *strcpy(char *dst, const char *src); /* copy string */
extern char *strncpy(char *dst, const char *src, size_t cnt); /* copy string with number of bytes */
extern int strcmp(const char *a, const char *b); /* compare strings */
extern int strncmp(const char *a, const char *b, size_t n); /* compare strings to nth byte */
extern char *strchr(const char *str, int ch); /* find character in string */
extern char *strchrs(const char *str, const char *chrs); /* find characters in string */

extern void *memset(void *dst, int ch, size_t cnt); /* fill a buffer with the specified character */
extern void *memcpy(void *dst, const void *src, size_t cnt); /* copy bytes */
extern int memcmp(const void *a, const void *b, size_t n); /* compare areas of memory to nth byte */

extern uint32_t strhash(const char *s); /* get string hash */

#endif /* STRING_H */
