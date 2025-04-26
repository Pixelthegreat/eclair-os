#include <string.h>

/* copy string */
extern char *strcpy(char *restrict s1, const char *restrict s2) {

	char *p = s1;

	while (*s2) *s1++ = *s2++;
	*s1++ = 0;
	return p;
}

/* get string length */
extern size_t strlen(const char *s) {

	size_t n = 0;
	while (*s++) n++;
	return n;
}

/* compare strings */
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
