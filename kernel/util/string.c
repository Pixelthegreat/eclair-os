/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/string.h>

/* get length of string */
extern size_t strlen(const char *str) {

	if (str == NULL) return 0;

	size_t sz = 0;
	while (*str++ != 0) sz++;
	return sz;
}

/* copy string */
extern char *strcpy(char *dst, const char *src) {

	size_t idx = 0;
	while (src[idx]) {

		dst[idx] = src[idx];
		idx++;
	}
	dst[idx] = 0;
	return dst;
}

/* copy string with max length */
extern char *strncpy(char *dst, const char *src, size_t cnt) {

	size_t idx = 0;
	while (idx < cnt && src[idx]) {

		dst[idx] = src[idx];
		idx++;
	}
	dst[idx] = 0;
	return dst;
}

/* compare strings */
extern int strcmp(const char *a, const char *b) {

	for (; *a == *b && *a; a++, b++);
	return *(unsigned char *)a - *(unsigned char *)b;
}

/* compare strings to nth byte */
extern int strncmp(const char *a, const char *b, size_t n) {

	if (!n--) return 0;
	for (; *a && *b && n && *a == *b; a++, b++, n--);
	return *(unsigned char *)a - *(unsigned char *)b;
}

/* find character in string */
extern char *strchr(const char *str, int ch) {

	while (*str) {
		if (*str == (char)ch)
			return (char *)str;
		str++;
	}
	return NULL;
}

/* find characters in string */
extern char *strchrs(const char *str, const char *chrs) {

	while (*str) {
		if (strchr(chrs, (int)*str))
			return (char *)str;
		str++;
	}
	return NULL;
}

/* fill a buffer with specified character */
extern void *memset(void *dst, int ch, size_t cnt) {

	if (!dst) return NULL;
	for (size_t i = 0; i < cnt; i++) ((uint8_t *)dst)[i] = (uint8_t)ch;
	return dst;
}

/* copy bytes */
extern void *memcpy(void *dst, const void *src, size_t cnt) {

	if (!dst) return NULL;
	for (size_t i = 0; i < cnt; i++) ((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
	return dst;
}

/* compare areas of memory to nth byte */
extern int memcmp(const void *a, const void *b, size_t n) {

	for (size_t i = 0; i < n; i++) {

		uint8_t ia = *(uint8_t *)a++;
		uint8_t ib = *(uint8_t *)b++;

		if (ia != ib) return (int)ia - (int)ib;
	}
	return 0;
}

/* get string hash */
static uint32_t powu32(uint32_t x, uint32_t y) {

	uint32_t res = 1;
	for (uint32_t i = 0; i < y; i++)
		res *= x;
	return res;
}

extern uint32_t strhash(const char *s) {

	uint32_t res = 0;
	if (!*s) return 0;
	uint32_t len = (uint32_t)strlen(s);

	for (uint32_t i = 0; i < len-1; i++)
		res += (uint32_t)s[i] * powu32(31, len-i-1);
	res += (uint32_t)s[len-1];

	return res;
}
