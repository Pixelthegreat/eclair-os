#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>

extern void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
extern char *strcpy(char *restrict s1, const char *restrict s2);
extern char *strncpy(char *restrict s1, const char *restrict s2, size_t n);

extern char *strcat(char *restrict s1, const char *restrict s2);
extern char *strncat(char *restrict s1, const char *restrict s2, size_t n);

extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);

extern char *strerror(int errnum);
extern size_t strlen(const char *str);

#endif /* _STRING_H */
