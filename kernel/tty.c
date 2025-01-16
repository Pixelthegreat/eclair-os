#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/varg.h>
#include <kernel/io/port.h>
#include <kernel/vfs/fs.h>
#include <kernel/tty.h>

static fs_node_t *ttydev = NULL;

static const char *hex = "0123456789abcdef";

/* initialize */
extern void tty_init(void) {
}

/* set character device */
extern void tty_set_device(fs_node_t *devn) {

	if (ttydev) fs_close(ttydev);

	ttydev = devn;
	fs_open(devn, FS_READ | FS_WRITE);
}

/* get character device */
extern fs_node_t *tty_get_device(void) {

	return ttydev;
}


/* write characters */
extern void tty_write(void *buf, size_t n) {

	if (!ttydev) return;
	fs_write(ttydev, 0, n, buf);
}

/* print string */
extern void tty_print(const char *s) {

	tty_write((void *)s, strlen(s));
}

/* print integer */
extern void tty_printi(int i) {

	if (!i) {
		tty_write("0", 1);
		return;
	} /* zero */

	int sign = i < 0;
	if (sign)  {
		tty_write("-", 1);
		i = -i; /* flip */
	}

	char buf[32]; /* reverse */
	int n = 0;
	while (i) {
		buf[n++] = (i % 10) + '0';
		i /= 10;
	}
	while (n--) tty_write(buf+n, 1);
}

/* print hexadecimal number */
extern void tty_printh(uint32_t h) {

	if (!h) {
		tty_write("0", 1);
		return;
	} /* zero */

	char buf[32];
	int n = 0;
	while (h) {
		buf[n++] = hex[h & 0xf];
		h >>= 4;
	}
	while (n--) tty_write(buf+n, 1);
}

/* print formatted */
extern void tty_printf(const char *fmt, ...) {

	va_list vlist;
	va_start(vlist, fmt);

	/* read characters (only supports 'c', 'd', 'x' and 's' format specifiers) */
	char c;
	while ((c = *fmt++) != 0) {

		if (c == '%') {
			c = *fmt++;

			/* character */
			if (c == 'c') {

				char pc = va_arg(vlist, char);
				tty_write(&pc, 1);
			}

			/* integer */
			else if (c == 'd') {

				int d = va_arg(vlist, int);
				tty_printi(d);
			}

			/* hex */
			else if (c == 'x') {

				uint32_t x = va_arg(vlist, uint32_t);
				tty_printh(x);
			}

			/* string */
			else if (c == 's') {

				const char *s = va_arg(vlist, const char *);
				tty_print(s);
			}
		}
		else tty_write(&c, 1);
	}
}

/* print newline */
extern void tty_printnl(void) {

	tty_write("\n", 1);
}
