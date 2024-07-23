#include <e.clair/types.h>
#include <e.clair/string.h>

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
	while (idx < (cnt-1) && src[idx]) {

		dst[idx] = src[idx];
		idx++;
	}
	dst[idx] = 0;
	return dst;
}

/* fill a buffer with specified character */
extern void *memset(void *dst, int ch, size_t cnt) {

	if (dst == NULL) return NULL;
	for (size_t i = 0; i < cnt; i++) ((uint8_t *)dst)[i] = (uint8_t)ch;
	return dst;
}
