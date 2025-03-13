#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/tty.h>
#include <kernel/task.h>
#include <kernel/vfs/fs.h>
#include <kernel/driver/device.h>
#include <kernel/driver/fb.h>
#include <kernel/driver/fbcon.h>

static fs_node_t *dev = NULL;

static uint32_t width, height;
static uint32_t idx = 0;

#define ESCBUFSZ 32
static char escbuf[32]; /* escape code buffer */
static size_t nescbuf = 0; /* number of chars in buffer */
static bool escready = false; /* ready to interpret sequence */

static const char *endstr = "mM"; /* escape code endings */

/* translations */
static const char ascii[DEVICE_KEYCODE_COUNT+1] = "?0123456789abcdefghijklmnopqrstuvwxyz`;\'()[]/\\,.=- ????????????????????????0123456789/*-+.?????????????";
static const char ascii_shift[DEVICE_KEYCODE_COUNT+1] = "?)!@#$%^&*(ABCDEFGHIJKLMNOPQRSTUVWXYZ~:\"(){}?|<>+_ ????????????????????????0123456789/*-+.?????????????";

/* ega text colors */
static const fb_color_t ega_colors[16] = {
	{0x00, 0x00, 0x00}, /* 0 */
	{0x00, 0x00, 0xaa}, /* 1 */
	{0x00, 0xaa, 0x00}, /* 2 */
	{0x00, 0xaa, 0xaa}, /* 3 */
	{0xaa, 0x00, 0x00}, /* 4 */
	{0xaa, 0x00, 0xaa}, /* 5 */
	{0xaa, 0x55, 0x00}, /* 6 */
	{0xaa, 0xaa, 0xaa}, /* 7 */
	{0x55, 0x55, 0x55}, /* 8 */
	{0x55, 0x55, 0xff}, /* 9 */
	{0x55, 0xff, 0x55}, /* 10 */
	{0x55, 0xff, 0xff}, /* 11 */
	{0xff, 0x55, 0x55}, /* 12 */
	{0xff, 0x55, 0xff}, /* 13 */
	{0xff, 0xff, 0x55}, /* 14 */
	{0xff, 0xff, 0xff}, /* 15 */
};

/* cursor shape */
#define CURSOR_WIDTH 9
#define CURSOR_HEIGHT 16

static uint8_t cursor[CURSOR_WIDTH * CURSOR_HEIGHT] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
};
static fb_color_t cursor_area_old[CURSOR_WIDTH * CURSOR_HEIGHT];
static bool cursor_show = false; /* cursor is shown */

#define CURSOR_UPDATE 500000000
static uint64_t timens = 0; /* current time */
static uint64_t timestart = 0; /* start time */

/* default colors */
#define DEFAULT_COLOR_INDEX 0x7
static const fb_color_t DEFAULT_COLOR = ega_colors[DEFAULT_COLOR_INDEX];

static fb_color_t bg = {0x00, 0x00, 0x00};
static fb_color_t fg = DEFAULT_COLOR;

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
	DEFAULT_COLOR_INDEX,
	DEFAULT_COLOR_INDEX,
};

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

static void fbcon_clear_cursor(uint32_t cidx);
static void fbcon_draw_cursor(uint32_t cidx);
static void fbcon_set_cursor(uint32_t old, uint32_t new);

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

/* scroll console */
static void fbcon_scroll(void) {

	if (idx < width * height) return;

	uint32_t old = idx;

	/* scroll display */
	size_t ys = fb_height - ((height-1) * fb_font.h);
	fb_scroll(ys);

	idx = width * (height-1);

	fbcon_set_cursor(old, idx);
}

/* clear cursor */
static void fbcon_clear_cursor(uint32_t cidx) {

	uint32_t cx = (cidx % width) * (fb_font.w + fb_font.hspace);
	uint32_t cy = (cidx / width) * fb_font.h;

	if (cursor_show) {

		cursor_show = false;

		for (uint32_t y = 0; y < CURSOR_HEIGHT; y++) {
			for (uint32_t x = 0; x < CURSOR_WIDTH; x++) {

				if (cursor[y * CURSOR_WIDTH + x])
					fb_set_pixel(cx+x, cy+y, cursor_area_old[y * CURSOR_WIDTH + x]);
			}
		}
	}
}

/* draw cursor */
static void fbcon_draw_cursor(uint32_t cidx) {

	uint32_t cx = (cidx % width) * (fb_font.w + fb_font.hspace);
	uint32_t cy = (cidx / width) * fb_font.h;

	if (!cursor_show) {

		cursor_show = true;

		for (uint32_t y = 0; y < CURSOR_HEIGHT; y++) {
			for (uint32_t x = 0; x < CURSOR_WIDTH; x++) {

				if (cursor[y * CURSOR_WIDTH + x]) {

					cursor_area_old[y * CURSOR_WIDTH + x] = fb_get_pixel(cx+x, cy+y);
					fb_set_pixel(cx+x, cy+y, DEFAULT_COLOR);
				}
			}
		}
	}
}

/* set cursor */
static void fbcon_set_cursor(uint32_t old, uint32_t new) {

	if (!cursor_show) return;

	fbcon_clear_cursor(old);
	fbcon_draw_cursor(new);
}

/* draw character */
static void fbcon_drawc(uint32_t index, char c) {

	uint32_t x = (index % width) * (fb_font.w + fb_font.hspace);
	uint32_t y = (index / width) * fb_font.h;

	char b[2] = {c, 0};
	fb_text(x, y, b, fg, bg);
}

/* print character */
static void fbcon_printc(char c) {

	if (c == '\n') {
		
		uint32_t old = idx;
		idx += width - (idx % width);

		fbcon_set_cursor(old, idx);
	}

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

		fbcon_drawc(idx, c);

		uint32_t old = idx++;
		fbcon_set_cursor(old, idx);
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
					bg = ega_colors[0];
					fg = DEFAULT_COLOR;
				}

				/* bright/bold */
				else if (num == 2) bright = true;

				/* dim/faint */
				else if (num == 1) bright = false;

				/* color */
				else if (num >= 30 && num < 50) {

					int isbg = (num-30) / 10;
					uint8_t col = stcolors[(num-30) % 10] + (bright? 8: 0);

					/* set color */
					if (isbg) bg = ega_colors[col];
					else fg = ega_colors[col];
				}
			}
		}
	}

	/* update scrolling */
	fbcon_scroll();
}

/* initialize */
extern void fbcon_init(void) {

	if (dev) return;

	width = fb_width / (fb_font.w + fb_font.hspace);
	height = fb_height / fb_font.h;

	/* create device */
	dev = fs_node_new(NULL, FS_CHARDEVICE);
	dev->read = fbcon_read;
	dev->write = fbcon_write;
}

/* initialize tty device */
extern void fbcon_init_tty(void) {

	fbcon_init();
	tty_add_device(dev);
}

/* read */
extern kssize_t fbcon_read(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf) {

	device_t *kbd = device_find_name("kybd", 0);
	if (!kbd) return -1;

	/* read */
	size_t nread = 0;
	uint32_t key = 0;
	bool shift = false;
	while ((key = device_char_read(kbd, true)) != DEVICE_KEYCODE_RETURN) {

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

			uint32_t old = idx--;
			fbcon_drawc(idx, ' ');
			fbcon_set_cursor(old, idx);
			continue;
		}

		/* add char */
		char c = shift? ascii_shift[key]: ascii[key];
		fbcon_printc(c);

		if (nread < nbytes-1)
			buf[nread] = c;
		nread++;
	}

	fbcon_printc('\n');
	buf[nread++] = 0;
	return nread > nbytes? nbytes: nread;
}

/* write */
extern kssize_t fbcon_write(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf) {

	for (size_t i = 0; i < nbytes; i++)
		fbcon_printc(((char *)buf)[i]);
	return (kssize_t)nbytes;
}

/* show/hide cursor */
extern void fbcon_flip_cursor(void) {

	if (cursor_show) fbcon_clear_cursor(idx);
	else fbcon_draw_cursor(idx);
}

/* update console */
extern void fbcon_update(void) {

	timens = task_get_global_time();
	uint64_t diff = timens - timestart;

	if (diff >= CURSOR_UPDATE) {

		timestart = timens;
		fbcon_flip_cursor();
	}
}
