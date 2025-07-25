/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <string.h>

/* copy data */
extern void *memcpy(void *restrict s1, const void *restrict s2, size_t n) {

	void *p = s1;
	while (n) {
		if (n > 4) {
			*(uint32_t *)s1 = *(uint32_t *)s2;
			s1 += 4;
			s2 += 4;
			n -= 4;
		}
		else {
			*(uint8_t *)s1++ = *(uint8_t *)s2++;
			n--;
		}
	}
	return p;
}

/* copy string */
extern char *strcpy(char *restrict s1, const char *restrict s2) {

	char *p = s1;

	while (*s2) *s1++ = *s2++;
	*s1++ = 0;
	return p;
}

extern char *strncpy(char *restrict s1, const char *restrict s2, size_t n) {

	char *p = s1;

	while (*s2 && --n) *s1++ = *s2++;
	*s1++ = 0;
	return p;
}

/* concatenate strings */
extern char *strcat(char *restrict s1, const char *restrict s2) {

	char *p = s1;

	s1 += strlen(s1);
	while (*s2) *s1++ = *s2++;
	*s1++ = 0;
	return p;
}

extern char *strncat(char *restrict s1, const char *restrict s2, size_t n) {

	char *p = s1;
	size_t len = strlen(s1);

	s1 += len;
	n -= len;
	while (*s2 && --n) *s1++ = *s2++;
	*s1++ = 0;
	return p;
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

/* compare strings */
extern int strcmp(const char *s1, const char *s2) {

	for (; *s1 == *s2 && *s1; s1++, s2++);

	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

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

extern char *strrchr(const char *s, int c) {

	const char *stop = NULL;
	while (*s) {

		if (*s == (char)c)
			stop = s;
		s++;
	}
	return (char *)stop;
}

/* get string length */
extern size_t strlen(const char *s) {

	size_t n = 0;
	while (*s++) n++;
	return n;
}
