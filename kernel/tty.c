#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <e.clair/io/port.h>
#include <e.clair/tty.h>

#define VGA_PORT_CTRL 0x3d4
#define VGA_PORT_DATA 0x3d5
#define VGA_OFFSET_LOW 0x0f
#define VGA_OFFSET_HIGH 0x0e

size_t tty_width = 80, tty_height = 25;
uint8_t *tty_mem = (uint8_t *)0xC03FF000;

static const char *hex = "0123456789abcdef";
static uint32_t idx = 0;

static void tty_scroll(void);

/* initialize */
extern void tty_init(void) {

	tty_clear();

	/* ... nothing else to do for the moment */
}

/* clear screen data */
extern void tty_clear(void) {

	memset(tty_mem, 0, tty_width * tty_height * 2);
}

/* write characters */
extern void tty_write(void *buf, size_t n) {

	for (int i = 0; i < n; i++)
		tty_printc(((const char *)buf)[i]);
}

/* print string */
extern void tty_print(const char *s) {

	tty_write((void *)s, strlen(s));
}

/* print character */
extern void tty_printc(char c) {

	if (c == '\n') idx += tty_width - (idx % tty_width);
	else {
		tty_mem[idx * 2] = c;
		tty_mem[idx * 2 + 1] = 0x7;
		idx++;
	}
	tty_scroll();

	/* set cursor position (will remove whenever proper vga driver comes around) */
	port_outb(VGA_PORT_CTRL, VGA_OFFSET_HIGH);
	port_outb(VGA_PORT_DATA, (uint8_t)(idx >> 8));
	port_outb(VGA_PORT_CTRL, VGA_OFFSET_LOW);
	port_outb(VGA_PORT_DATA, (uint8_t)(idx & 0xff));
}

/* print integer */
extern void tty_printi(int i) {

	if (!i) {
		tty_printc('0');
		return;
	} /* zero */

	int sign = i < 0;
	if (sign)  {
		tty_printc('-');
		i = -i; /* flip */
	}

	char buf[32]; /* reverse */
	int n = 0;
	while (i) {
		buf[n++] = (i % 10) + '0';
		i /= 10;
	}
	while (n--) tty_printc(buf[n]);
}

/* print hexadecimal number */
extern void tty_printh(uint32_t h) {

	if (!h) {
		tty_printc('0');
		return;
	} /* zero */

	char buf[32];
	int n = 0;
	while (h) {
		buf[n++] = hex[h & 0xf];
		h >>= 4;
	}
	while (n--) tty_printc(buf[n]);
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
				tty_printc(pc);
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
		else tty_printc(c);
	}
}

/* print newline */
extern void tty_printnl(void) {

	tty_printc('\n');
}

/* update scrolling */
static void tty_scroll(void) {

	if (idx < tty_width * tty_height) return;

	/* copy */
	size_t lr = tty_width * (tty_height - 1); /* last row */
	for (uint32_t i = 0; i < lr * 2; i++)
		tty_mem[i] = tty_mem[i + tty_width * 2];

	/* clear bottom row */
	memset(tty_mem + lr * 2, 0, tty_width * 2);
	idx = lr;
}
