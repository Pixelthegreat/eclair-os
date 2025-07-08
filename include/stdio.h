/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _STDIO_H
#define _STDIO_H 1

#include <stddef.h>
#include <stdarg.h>

typedef long fpos_t;

typedef struct __LIBC_FILE FILE;

#define _IOFBF 1
#define _IOLBF 2
#define _IONBF 3

#define BUFSIZ 256

#define EOF ((int)-1)

#define FOPEN_MAX 128
#define FILENAME_MAX 4096

#define L_tmpnam 32

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define TMP_MAX 2048

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

extern int fclose(FILE *stream);
extern int fflush(FILE *stream);
extern FILE *fopen(const char *restrict filename, const char *restrict mode);

extern int fprintf(FILE *restrict stream, const char *restrict format, ...);
extern int printf(const char *restrict format, ...);
extern int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
extern int sprintf(char *restrict s, const char *restrict format, ...);
extern int vfprintf(FILE *restrict stream, const char *restrict format, va_list arg);
extern int vprintf(const char *restrict format, va_list arg);
extern int vsnprintf(char *restrict s, size_t n, const char *restrict format, va_list arg);
extern int vsprintf(char *restrict s, const char *restrict format, va_list arg);

extern int fgetc(FILE *stream);
extern char *fgets(char *restrict s, int n, FILE *restrict stream);
extern int fputc(int c, FILE *stream);
extern int fputs(const char *restrict s, FILE *restrict stream);

extern size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
extern size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
extern int fseek(FILE *stream, long int offset, int whence);
extern long int ftell(FILE *stream);

extern int ferror(FILE *stream);

#endif /* _STDIO_H */
