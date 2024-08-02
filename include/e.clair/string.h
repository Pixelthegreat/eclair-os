#ifndef STRING_H
#define STRING_H

#include <e.clair/types.h> /* size_t */

extern size_t strlen(const char *str); /* get length of string */
extern char *strcpy(char *dst, const char *src); /* copy string */
extern char *strncpy(char *dst, const char *src, size_t cnt); /* copy string with number of bytes */
extern int strcmp(const char *a, const char *b); /* compare strings */
extern int strncmp(const char *a, const char *b, size_t n); /* compare strings to nth byte */
extern char *strchr(const char *str, int ch); /* find character in string */

extern void *memset(void *dst, int ch, size_t cnt); /* fill a buffer with the specified character */
extern void *memcpy(void *dst, const void *src, size_t cnt); /* copy bytes */

#endif /* STRING_H */
