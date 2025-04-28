#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>

extern char *strcpy(char *restrict s1, const char *restrict s2);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strchr(const char *s, int c);

extern char *strerror(int errnum);
extern size_t strlen(const char *str);

#endif /* _STRING_H */
