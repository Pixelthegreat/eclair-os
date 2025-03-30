#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/tty.h>
#include <kernel/io/port.h>
#include <kernel/vfs/fs.h>
#include <kernel/driver/device.h>
#include <kernel/driver/vgacon.h>

static fs_node_t *dev = NULL;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_PORT_CONTROL 0x3d4
#define VGA_PORT_DATA 0x3d5
#define VGA_OFFSET_LOW 0x0f
#define VGA_OFFSET_HIGH 0x0e

#define DEFAULT_COLOR 0x7
static uint8_t color = DEFAULT_COLOR;

static uint8_t *vidmem = (uint8_t *)0xC03FF000;
static uint32_t idx = 0;

#define ESCBUFSZ 32
static char escbuf[32]; /* escape code buffer */
static size_t nescbuf = 0; /* number of chars in buffer */
static bool escready = false; /* ready to interpret sequence */

static const char *endstr = "mM"; /* escape code endings */

/* translations */
static const char ascii[DEVICE_KEYCODE_COUNT+1] = "?0123456789abcdefghijklmnopqrstuvwxyz`;\'()[]/\\,.=- ????????????????????????0123456789/*-+.?????????????";
static const char ascii_shift[DEVICE_KEYCODE_COUNT+1] = "?)!@#$%^&*(ABCDEFGHIJKLMNOPQRSTUVWXYZ~:\"(){}?|<>+_ ????????????????????????0123456789/*-+.?????????????";

/* style colors */
static uint8_t stcolors[10] = {
	0x0, /* black */
	0x4, /* red */
	0x2, /* green */
	0x6, /* yellow */
	0x1, /* blue */
	0x5, /* magenta */
	0x3, /* cyan */
	0x7, /* white */
	DEFAULT_COLOR,
	DEFAULT_COLOR,
};

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

/* read integer from escape code buffer */
static uint32_t read_esc_int(uint32_t p, int *d) {

	char c = escbuf[p];
	int r = 0;
	for (; IS_DIGIT(c); c = escbuf[++p])
		r = r * 10 + (c - '0');
	*d = r;

	if (c == ';') p++; /* argument separator */
	return p;
}

/* update scrolling */
static void vgacon_scroll(void) {

	if (idx < VGA_WIDTH * VGA_HEIGHT) return;

	/* copy */
	size_t lr = VGA_WIDTH * (VGA_HEIGHT-1); /* up to last row */
	for (uint32_t i = 0; i < lr * 2; i++)
		vidmem[i] = vidmem[i + VGA_WIDTH * 2];

	/* clear bottom row */
	for (uint32_t i = lr; i < VGA_WIDTH * VGA_HEIGHT; i++) {
		vidmem[i * 2] = 0;
		vidmem[i * 2 + 1] = DEFAULT_COLOR;
	}
	idx = lr;
}

/* set cursor position */
static void vgacon_set_cursor(void) {

	port_outb(VGA_PORT_CONTROL, VGA_OFFSET_HIGH);
	port_outb(VGA_PORT_DATA, (idx >> 8) & 0xff);
	port_outb(VGA_PORT_CONTROL, VGA_OFFSET_LOW);
	port_outb(VGA_PORT_DATA, idx & 0xff);
}

/* clear screen */
static void vgacon_clear(void) {

	for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {

		vidmem[i * 2] = 0;
		vidmem[i * 2 + 1] = color;
	}
}

/* print character */
static void vgacon_printc(char c) {

	if (c == '\n') idx += VGA_WIDTH - (idx % VGA_WIDTH);

	/* escape code */
	else if (c == 0x1b) {
		nescbuf = 1;
		escbuf[0] = 0x1b;
	}
	else if (nescbuf == 1 && c == '[') escbuf[nescbuf++] = '[';
	else if (nescbuf >= 2) {

		/* ready */
		if (nescbuf >= ESCBUFSZ-1) {

			escready = true;
			escbuf[nescbuf] = 0;
		}
		else {

			escbuf[nescbuf++] = c;
			if (strchr(endstr, c)) {

				escready = true;
				escbuf[nescbuf] = 0;
			}
		}
	}

	/* normal character */
	else {

		nescbuf = 0; /* reset escape buffer count */
		escready = false;

		vidmem[idx * 2] = c;
		vidmem[idx * 2 + 1] = color;
		idx++;
	}

	/* interpret escape code */
	if (escready) {

		char cmd = escbuf[nescbuf-1];
		nescbuf = 0;
		escready = false;

		/* change style */
		if (cmd == 'm') {

			int num = 1;
			uint32_t pos = 2;
			bool bright = false;
			while (escbuf[pos] && escbuf[pos] != cmd) {

				pos = read_esc_int(pos, &num);

				/* reset */
				if (num == 0) {
					bright = false;
					color = DEFAULT_COLOR;
				}

				/* bright/bold */
				else if (num == 2) bright = true;

				/* dim/faint */
				else if (num == 1) bright = false;

				/* color */
				else if (num >= 30 && num < 50) {

					int bg = (num-30) / 10;
					uint8_t col = stcolors[(num-30) % 10] + (bright? 8: 0);

					/* set color */
					color &= 0xf << (bg? 0: 4);
					color |= col << (bg? 4: 0);
				}
			}
		}
	}

	/* update scrolling */
	vgacon_scroll();
	vgacon_set_cursor();
}

/* read */
extern kssize_t vgacon_read(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf) {

	device_t *kbd = devclass_keyboard.first;
	if (!kbd) return -1;

	/* read */
	size_t nread = 0;
	uint32_t key = 0;
	bool shift = false;
	while ((key = device_keyboard_getkey_block(kbd)) != DEVICE_KEYCODE_RETURN) {

		uint32_t keyrel = key & DEVICE_KEYCODE_RELEASE;
		key &= DEVICE_KEYCODE_CODE;

		if (key >= DEVICE_KEYCODE_COUNT) continue;

		/* enable/disable shift set */
		else if (key == DEVICE_KEYCODE_LEFT_SHIFT || key == DEVICE_KEYCODE_RIGHT_SHIFT) {

			if (keyrel) shift = false;
			else shift = true;
			continue;
		}

		/* don't care about key being released */
		else if (keyrel) continue;

		/* remove char */
		else if (key == DEVICE_KEYCODE_BACKSPACE) {

			if (!nread) continue;

			nread--;
			if (nread < nbytes-1) buf[nread] = 0;

			vidmem[--idx * 2] = 0;
			vgacon_set_cursor();
			continue;
		}

		/* add char */
		char c = shift? ascii_shift[key]: ascii[key];
		vgacon_printc(c);

		if (nread < nbytes-1) {

			buf[nread] = c;
		}
		nread++;
	}

	vgacon_printc('\n');
	buf[nread++] = 0;
	return nread > nbytes? nbytes: nread;
}

/* write */
extern kssize_t vgacon_write(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf) {

	for (size_t i = 0; i < nbytes; i++)
		vgacon_printc(((char *)buf)[i]);
	return (kssize_t)nbytes;
}

/* base init */
extern void vgacon_init(void) {

	if (dev) return;

	dev = fs_node_new(NULL, FS_CHARDEVICE);
	dev->read = vgacon_read;
	dev->write = vgacon_write;

	vgacon_clear();
}

/* set kernel tty device */
extern void vgacon_set_tty(void) {

	tty_add_device(dev);
}

/* init and set tty device */
extern void vgacon_init_tty(void) {

	vgacon_init();
	vgacon_set_tty();
}
