#include <e.clair/types.h>
#include <e.clair/string.h>

/* get length of string */
extern size_t strlen(const char *str) {

	if (str == NULL) return 0;

	size_t sz = 0;
	while (*str++ != 0) sz++;
	return sz;
}

/* fill a buffer with specified character */
extern void *memset(void *dst, int ch, size_t cnt) {

	if (dst == NULL) return NULL;
	for (size_t i = 0; i < cnt; i++) ((uint8_t *)dst)[i] = (uint8_t)ch;
	return dst;
}
