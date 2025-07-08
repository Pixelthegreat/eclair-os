/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stddef.h>

typedef int div_t;
typedef long int ldiv_t;
typedef long long int lldiv_t;

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define RAND_MAX 2147483647

/* memory management */
extern void *calloc(size_t nmemb, size_t size);
extern void free(void *ptr);
extern void *malloc(size_t size);
extern void *realloc(void *ptr, size_t size);

/* environment */
extern void abort(void);
extern int atexit(void (*func)(void));
extern void exit(int status);
extern void _Exit(int status);
extern char *getenv(const char *name);
extern int system(const char *string);

#endif /* _STDLIB_H */
