#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <ec.h>

static const char *hexchars = "0123456789abcdef";
static const char *hexchars_upper = "0123456789ABCDEF";

#define MFILE 'f'
#define MBUF 's'
struct finfo {
	int mode; /* MFILE or MBUF */
	union {
		FILE *file;
		char *buffer;
	};
	size_t size;
	size_t pos;
	va_list args;
};

/* write characters */
static int finfowr(struct finfo *info, const char *buf, size_t nbytes) {

	if (info->mode == MFILE) {

		info->pos += (size_t)fwrite(buf, 1, nbytes, info->file);
		if (ferror(info->file)) return -1;
	}
	else if (info->mode == MBUF) {

		for (size_t i = 0; i < nbytes && info->pos < info->size-1; i++)
			info->buffer[info->pos++] = buf[i];
	}
	return 0;
}

/* interpret format specifier */
#define FMT_DEF 0
#define FMT_INT 0
#define FMT_CHAR 1
#define FMT_SHORT 2
#define FMT_LONG 3
#define FMT_LLONG 4
#define FMT_INTPTR 5
#define FMT_SIZE 6
#define FMT_PTRDIFF 7
#define FMT_LDOUBLE 8

#define FMT_CONV_INT 0 /* d,i */
#define FMT_CONV_OCTAL 1 /* o */
#define FMT_CONV_UINT 2 /* u */
#define FMT_CONV_HEX 3 /* x,X */
#define FMT_CONV_DOUBLE 4 /* f,F */
#define FMT_CONV_EXP 5 /* e,E */
#define FMT_CONV_EXPAUTO 6 /* g,G */
#define FMT_CONV_HEXEXP 7 /* a,A */
#define FMT_CONV_CHAR 8 /* c */
#define FMT_CONV_STR 9 /* s */
#define FMT_CONV_PTR 10 /* p */
#define FMT_CONV_NCHARS 11 /* n */

#define FMT_CONV_COUNT 12

struct fspec {
	int paddir; /* padding direction (right aligned = 0, left aligned = 1) */
	char psignchar; /* print positive sign (0/none, '+' or ' ') */
	int width; /* field width */
	int len; /* length modifier */
	int conv; /* conversion specifier */
	char convc; /* conversion specifier character */
};
#define FSPEC_INIT {0, 0, 0, 0, 0}

static const char *readfspec(struct fspec *spec, const char *fmt) {

	spec->conv = -1;
	char c;
	while ((c = *fmt) != 0) {
		fmt++;
		
		if (c == '-') spec->paddir = 1;
		else if (c == '+') spec->psignchar = '+';
		else if (c == ' ' && !spec->psignchar) spec->psignchar = ' ';
		else if (c == '#') continue;
		else if (c >= '0' && c <= '9') spec->width = (spec->width * 10) + (c - '0');

		/* length modifiers */
		else if (c == 'h') {

			if (spec->len == FMT_INT) spec->len = FMT_SHORT;
			else if (spec->len == FMT_SHORT) spec->len = FMT_CHAR;
		}
		else if (c == 'l') {

			if (spec->len == FMT_INT) spec->len = FMT_LONG;
			else if (spec->len == FMT_LONG) spec->len = FMT_LLONG;
		}
		else if (c == 'j' && spec->len == FMT_INT) spec->len = FMT_INTPTR;
		else if (c == 'z' && spec->len == FMT_INT) spec->len = FMT_SIZE;
		else if (c == 't' && spec->len == FMT_INT) spec->len = FMT_PTRDIFF;
		else if (c == 'L' && spec->len == FMT_INT) spec->len = FMT_LDOUBLE;

		else break;
	}

	/* get conversion specifier */
	spec->convc = c;
	if (c == 'd' || c == 'i') spec->conv = FMT_CONV_INT;
	else if (c == 'o') spec->conv = FMT_CONV_OCTAL;
	else if (c == 'u') spec->conv = FMT_CONV_UINT;
	else if (c == 'x' || c == 'X') spec->conv = FMT_CONV_HEX;
	else if (c == 'f' || c == 'F') spec->conv = FMT_CONV_DOUBLE;
	else if (c == 'e' || c == 'E') spec->conv = FMT_CONV_EXP;
	else if (c == 'g' || c == 'G') spec->conv = FMT_CONV_EXPAUTO;
	else if (c == 'a' || c == 'A') spec->conv = FMT_CONV_HEXEXP;
	else if (c == 'c') spec->conv = FMT_CONV_CHAR;
	else if (c == 's') spec->conv = FMT_CONV_STR;
	else if (c == 'p') spec->conv = FMT_CONV_PTR;
	else if (c == 'n') spec->conv = FMT_CONV_NCHARS;
	return fmt;
}


static int finfowr_uint(struct finfo *info, struct fspec *spec, va_list args);
static int finfowr_double(struct finfo *info, struct fspec *spec, va_list args);

/* write int */
static int finfowr_int(struct finfo *info, struct fspec *spec, va_list args) {

	long long v; /* value */
	switch (spec->len) {
		case FMT_LONG:
			v = (long long)va_arg(info->args, long);
			break;
		case FMT_LLONG:
			v = va_arg(info->args, long long);
			break;
		case FMT_INTPTR:
			v = (long long)va_arg(info->args, intptr_t);
			break;
		case FMT_PTRDIFF:
			v = (long long)va_arg(info->args, ptrdiff_t);
			break;
		case FMT_SIZE:
			return finfowr_uint(info, spec, args);
		case FMT_LDOUBLE:
			return finfowr_double(info, spec, args);
		default:
			v = (long long)va_arg(info->args, int);
	}

	/* zero */
	if (!v) {

		if (finfowr(info, "0", 1) < 0)
			return -1;
		return 0;
	}

	int sign = v < 0;
	if (sign) {

		if (finfowr(info, "-", 1) < 0)
			return -1;
		v = -v;
	}
	else if (spec->psignchar && finfowr(info, &spec->psignchar, 1) < 0)
		return -1;

	char buf[32];
	size_t p = 32;
	while (v) {
		
		buf[--p] = (char)(v % 10) + '0';
		v /= 10;
	}

	if (finfowr(info, buf+p, 32-p) < 0)
		return -1;
	return 0;
}

/* write octal */
static int finfowr_octal(struct finfo *info, struct fspec *spec, va_list args) {

	unsigned long long v; /* value */
	switch (spec->len) {
		case FMT_LONG:
			v = (unsigned long long)va_arg(info->args, unsigned long);
			break;
		case FMT_LLONG:
			v = va_arg(info->args, unsigned long long);
			break;
		case FMT_INTPTR:
			v = (unsigned long long)va_arg(info->args, uintptr_t);
			break;
		case FMT_PTRDIFF:
			return -1;
		case FMT_SIZE:
			v = (unsigned long long)va_arg(info->args, size_t);
		case FMT_LDOUBLE:
			return finfowr_double(info, spec, args);
		default:
			v = (unsigned long long)va_arg(info->args, unsigned int);
	}

	/* zero */
	if (!v) {

		if (finfowr(info, "0", 1) < 0)
			return -1;
		return 0;
	}
	
	if (spec->psignchar && finfowr(info, &spec->psignchar, 1) < 0)
		return -1;

	char buf[32];
	size_t p = 32;
	while (v) {
		
		buf[--p] = (char)(v & 0x7) + '0';
		v >>= 3;
	}

	if (finfowr(info, buf+p, 32-p) < 0)
		return -1;
	return 0;
}

/* write uint */
static int finfowr_uint(struct finfo *info, struct fspec *spec, va_list args) {

	unsigned long long v; /* value */
	switch (spec->len) {
		case FMT_LONG:
			v = (unsigned long long)va_arg(info->args, unsigned long);
			break;
		case FMT_LLONG:
			v = va_arg(info->args, unsigned long long);
			break;
		case FMT_INTPTR:
			v = (unsigned long long)va_arg(info->args, uintptr_t);
			break;
		case FMT_PTRDIFF:
			return -1;
		case FMT_SIZE:
			v = (unsigned long long)va_arg(info->args, size_t);
		case FMT_LDOUBLE:
			return finfowr_double(info, spec, args);
		default:
			v = (unsigned long long)va_arg(info->args, unsigned int);
	}

	/* zero */
	if (!v) {

		if (finfowr(info, "0", 1) < 0)
			return -1;
		return 0;
	}
	
	if (spec->psignchar && finfowr(info, &spec->psignchar, 1) < 0)
		return -1;

	char buf[32];
	size_t p = 32;
	while (v) {
		
		buf[--p] = (char)(v % 10) + '0';
		v /= 10;
	}

	if (finfowr(info, buf+p, 32-p) < 0)
		return -1;
	return 0;
}

/* write hex */
static int finfowr_hex(struct finfo *info, struct fspec *spec, va_list args) {

	unsigned long long v; /* value */
	switch (spec->len) {
		case FMT_LONG:
			v = (unsigned long long)va_arg(info->args, unsigned long);
			break;
		case FMT_LLONG:
			v = va_arg(info->args, unsigned long long);
			break;
		case FMT_INTPTR:
			v = (unsigned long long)va_arg(info->args, uintptr_t);
			break;
		case FMT_PTRDIFF:
			return -1;
		case FMT_SIZE:
			v = (unsigned long long)va_arg(info->args, size_t);
		case FMT_LDOUBLE:
			return finfowr_double(info, spec, args);
		default:
			v = (unsigned long long)va_arg(info->args, unsigned int);
	}

	/* zero */
	if (!v) {

		if (finfowr(info, "0", 1) < 0)
			return -1;
		return 0;
	}
	
	if (spec->psignchar && finfowr(info, &spec->psignchar, 1) < 0)
		return -1;

	char buf[32];
	size_t p = 32;
	while (v) {
		
		buf[--p] = (spec->convc == 'X'? hexchars_upper: hexchars)[v & 0xf];
		v >>= 4;
	}

	if (finfowr(info, buf+p, 32-p) < 0)
		return -1;
	return 0;
}

/* write double */
static int finfowr_double(struct finfo *info, struct fspec *spec, va_list args) {

	return -1;
}

/* write exp */
static int finfowr_exp(struct finfo *info, struct fspec *spec, va_list args) {

	return -1;
}

/* write expauto */
static int finfowr_expauto(struct finfo *info, struct fspec *spec, va_list args) {

	return -1;
}

/* write hexexp */
static int finfowr_hexexp(struct finfo *info, struct fspec *spec, va_list args) {

	return -1;
}

/* write char */
static int finfowr_char(struct finfo *info, struct fspec *spec, va_list args) {

	char c = (char)va_arg(info->args, int);
	if (finfowr(info, &c, 1) < 0)
		return -1;
	return 0;
}

/* write str */
static int finfowr_str(struct finfo *info, struct fspec *spec, va_list args) {

	const char *str = va_arg(info->args, const char *);
	if (!str) return -1;

	size_t len = strlen(str);
	size_t width = (size_t)spec->width;

	if (width && !spec->paddir && len < width) {
		for (size_t i = 0; i < width-len; i++) {
			if (finfowr(info, " ", 1) < 0)
				return -1;
		}
	}

	if (finfowr(info, str, len) < 0)
		return -1;

	if (width && spec->paddir && len < width) {
		for (size_t i = 0; i < width-len; i++) {
			if (finfowr(info, " ", 1) < 0)
				return -1;
		}
	}
	return 0;
}

/* write ptr */
static int finfowr_ptr(struct finfo *info, struct fspec *spec, va_list args) {

	if (finfowr(info, "0x", 2) < 0)
		return -1;
	spec->len = FMT_INTPTR;
	return finfowr_hex(info, spec, args);
}

/* store number of characters written as of this format specifier */
static int finfowr_nchars(struct finfo *info, struct fspec *spec, va_list args) {

	int *p = va_arg(info->args, int *);

	*p = (int)info->pos;
	return 0;
}

/* write value based off of format specifier */
static int (*finfowr_conv[FMT_CONV_COUNT])(struct finfo *, struct fspec *, va_list) = {
	finfowr_int,
	finfowr_octal,
	finfowr_uint,
	finfowr_hex,
	finfowr_double,
	finfowr_exp,
	finfowr_expauto,
	finfowr_hexexp,
	finfowr_char,
	finfowr_str,
	finfowr_ptr,
	finfowr_nchars,
};

static int finfowr_fspec(struct finfo *info, struct fspec *spec, va_list args) {

	if (spec->conv < 0 || spec->conv >= FMT_CONV_COUNT || !finfowr_conv[spec->conv])
		return 0;

	return finfowr_conv[spec->conv](info, spec, args);
}

/* internal multi-use case printf */
static int internprintf(struct finfo *info, const char *fmt, va_list args) {

	info->args = args;

	char c;
	const char *nfmt = fmt; /* start position of normal chars */
	size_t nlen = 0; /* length of normal chars */
	while ((c = *fmt) != 0) {
		fmt++;
		if (c == '%') {

			/* write unwritten characters */
			if (nlen && nfmt) {

				if (finfowr(info, nfmt, nlen) < 0)
					return -1;
				nlen = 0;
			}

			struct fspec spec = FSPEC_INIT;
			fmt = readfspec(&spec, fmt);
			nfmt = fmt;

			if (spec.conv < 0) continue;
			else if (finfowr_fspec(info, &spec, args) < 0)
				return -1;
		}

		else nlen++;
	}

	/* write final characters */
	if (nlen && nfmt && finfowr(info, nfmt, nlen) < 0)
		return -1;
	if (info->mode == MBUF) info->buffer[info->pos] = 0;
	return (int)info->pos;
}

/* libc printf variations */
extern int fprintf(FILE *restrict stream, const char *restrict format, ...) {

	va_list args;
	va_start(args, format);
	int res = vfprintf(stream, format, args);
	va_end(args);

	return res;
}

extern int printf(const char *restrict format, ...) {

	va_list args;
	va_start(args, format);
	int res = vfprintf(stdout, format, args);
	va_end(args);

	return res;
}

extern int snprintf(char *restrict s, size_t n, const char *restrict format, ...) {

	va_list args;
	va_start(args, format);
	int res = vsnprintf(s, n, format, args);
	va_end(args);

	return res;
}

extern int sprintf(char *restrict s, const char *restrict format, ...) {

	va_list args;
	va_start(args, format);
	int res = vsprintf(s, format, args);
	va_end(args);

	return res;
}

extern int vfprintf(FILE *restrict stream, const char *restrict format, va_list arg) {

	struct finfo info = {
		.mode = MFILE,
		.file = stream,
		.size = 0,
		.pos = 0,
	};
	return internprintf(&info, format, arg);
}

extern int vprintf(const char *restrict format, va_list arg) {

	return vfprintf(stdout, format, arg);
}

extern int vsnprintf(char *restrict s, size_t n, const char *restrict format, va_list arg) {

	struct finfo info = {
		.mode = MBUF,
		.buffer = s,
		.size = n,
		.pos = 0,
	};
	return internprintf(&info, format, arg);
}

extern int vsprintf(char *restrict s, const char *restrict format, va_list arg) {

	return vsnprintf(s, SIZE_MAX, format, arg);
}
