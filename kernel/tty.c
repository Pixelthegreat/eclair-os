#include <stdarg.h>
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/io/port.h>
#include <kernel/vfs/fs.h>
#include <kernel/vfs/devfs.h>
#include <kernel/driver/uart.h>
#include <kernel/tty.h>

#define MAX_DEVS 8
static fs_node_t *ttydev[MAX_DEVS];
static int nttydev = 0;

static const char *hex = "0123456789abcdef";

/* write vfs node */
static kssize_t write_fs(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	tty_write((void *)buf, nbytes);
	return (kssize_t)nbytes;
}

/* initialize */
extern void tty_init(void) {
}

/* initialize vfs nodes */
extern void tty_init_devfs(void) {

	fs_node_t *node = fs_node_new(NULL, FS_CHARDEVICE);
	
	if (nttydev) node->read = ttydev[0]->read;
	node->write = write_fs;

	devfs_add_node("tty", node);
}

/* add character device */
extern void tty_add_device(fs_node_t *devn) {

	if (nttydev >= MAX_DEVS) return;

	int i = nttydev++;
	ttydev[i] = devn;
	fs_open(devn, FS_READ | FS_WRITE);
}

/* get character device */
extern fs_node_t *tty_get_device(int i) {

	if (i < 0 || i >= nttydev) return NULL;

	return ttydev[i];
}

/* write characters */
extern void tty_write(void *buf, size_t n) {

	for (int i = 0; i < nttydev; i++)
		fs_write(ttydev[i], 0, n, buf);
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

/* variadic printf */
extern void tty_vprintf(const char *fmt, va_list args) {

	/* read characters (only supports 'c', 'd', 'x' and 's' format specifiers) */
	char c;
	while ((c = *fmt++) != 0) {

		if (c == '%') {
			c = *fmt++;

			/* character */
			if (c == 'c') {

				char pc = (char)va_arg(args, int);
				tty_write(&pc, 1);
			}

			/* integer */
			else if (c == 'd') {

				int d = va_arg(args, int);
				tty_printi(d);
			}

			/* hex */
			else if (c == 'x') {

				uint32_t x = va_arg(args, uint32_t);
				tty_printh(x);
			}

			/* string */
			else if (c == 's') {

				const char *s = va_arg(args, const char *);
				tty_print(s);
			}
		}
		else tty_write(&c, 1);
	}
}

/* print formatted */
extern void tty_printf(const char *fmt, ...) {

	va_list args;
	va_start(args, fmt);
	tty_vprintf(fmt, args);
	va_end(args);
}

/* print newline */
extern void tty_printnl(void) {

	tty_write("\n", 1);
}
