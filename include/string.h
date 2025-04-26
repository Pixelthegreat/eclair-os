#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>

extern char *strcpy(char *restrict s1, const char *restrict s2);
extern size_t strlen(const char *str);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strchr(const char *s, int c);

#endif /* _STRING_H */
