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
		if ((int)*str == ch)
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
