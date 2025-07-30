/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ec.h>
#include <ec/device.h>
#include <wm/screen.h>

static int fd = -1; /* framebuffer fd */
static ecio_fbinfo_t fbinfo; /* framebuffer info */
static void *fb_addr = NULL; /* framebuffer address */
static int ocurx, ocury, curx, cury; /* cursor positions */
static bool drawcur; /* draw cursor */

/* cursor data */
#define CURSOR_HEIGHT 11
static uint8_t cursor[CURSOR_HEIGHT] = {
	0b10000000,
	0b11000000,
	0b11100000,
	0b11110000,
	0b11111000,
	0b11111100,
	0b11111110,
	0b11111111,
	0b11111000,
	0b10011100,
	0b00001100,
};

image_t screen_image; /* main screen image */

/* draw cursor */
static void draw_cursor(int x, int y) {

	for (int py = 0; py < CURSOR_HEIGHT; py++) {

		if (y+py >= (int)fbinfo.height) break;
		for (int px = 0; px < CURSOR_HEIGHT; px++) {

			if (x+px >= (int)fbinfo.width) break;

			uint8_t bit = (cursor[py] >> (uint8_t)(7-px)) & 0x1;
			for (uint32_t i = 0; i < fbinfo.depth_bytes; i++)
				*(uint8_t *)(fb_addr + (y+py) * (int)fbinfo.pitch + (x+px) * (int)fbinfo.depth_bytes + (int)i) ^= (0xff * bit);
		}
	}
}

/* initialize image */
extern void image_init(image_t *image, size_t width, size_t height) {

	image->width = width;
	image->height = height;
	image->depth_bytes = (size_t)fbinfo.depth_bytes;
	image->pitch = width * image->depth_bytes;
	image->data = malloc(image->pitch * image->height);
}

/* fill image with color */
extern void image_fill(image_t *image, color_t color) {

	uint8_t buf[4];
	screen_convert_color(buf, color);

	if (!image->height) return;

	for (size_t x = 0; x < image->width; x++) {
		for (size_t i = 0; i < image->depth_bytes; i++)
			((uint8_t *)image->data)[x * image->depth_bytes + i] = buf[i];
	}
	for (size_t y = 1; y < image->height; y++)
		memcpy(image->data + y * image->pitch, image->data, image->pitch);
}

/* copy image area */
extern void image_copy_area(image_t *dest, image_t *source, int x, int y, int sx, int sy, int sw, int sh) {

	if (x < 0) {
		sw -= x;
		x = 0;
		if (sw < 0) return;
	}
	else if ((x + sw) >= (int)dest->width) {
		sw = (int)dest->width - x;
		if (sw < 0) return;
	}
	if (sx < 0) {
		sw -= sx;
		sx = 0;
		if (sw < 0) return;
	}
	else if ((sx + sw) >= (int)source->width) {
		sw = (int)source->width - sx;
		if (sw < 0) return;
	}

	/* copy image data */
	for (int py = 0; py < sh; py++) {

		int ay = py + sy;
		int dy = py + y;
		if (ay < 0 || ay >= (int)source->height || dy < 0 || dy >= (int)dest->height)
			break;

		memcpy(dest->data + (y + py) * (int)dest->pitch + x * (int)dest->depth_bytes,
		       source->data + (sy + py) * (int)source->pitch + sx * (int)source->depth_bytes,
		       (size_t)sw * source->depth_bytes);
	}
}

/* resize image */
extern void image_resize(image_t *image, size_t width, size_t height) {

	image->width = width;
	image->height = height;
	image->pitch = width * image->depth_bytes;
	free(image->data);
	image->data = malloc(image->pitch * height);
}

/* set image data */
extern void image_set_data(image_t *image, uint32_t format, size_t offset, size_t size, uint8_t *data) {

	size_t p = 0;
	uint8_t buf[4];
	for (size_t i = offset; i < offset+size; i++, (p += (size_t)format+2)) {

		size_t y = i / image->width;
		size_t x = i % image->width;

		if (y >= image->height)
			return;

		color_t color = {*(data+p), *(data+p+1), *(data+p+2)};
		screen_convert_color(buf, color);

		memcpy(image->data + y * image->pitch + x * image->depth_bytes, buf, image->depth_bytes);
	}
}

/* destroy image */
extern void image_destroy(image_t *image) {

	if (image->data) free(image->data);
	image->data = NULL;
}

/* initialize screen */
extern int screen_init(void) {

	ec_ioctl(0, ECIO_TTY_CURSOR, 1); /* stop cursor flashing */

	fd = ec_open("/dev/fb0", ECF_READ, 0);
	if (fd < 0) {

		fprintf(stderr, "Can't open '/dev/fb0': %s\n", strerror(errno));
		return -1;
	}

	if (ec_ioctl(fd, ECIO_FB_GETINFO, (uintptr_t)&fbinfo) < 0) {

		fprintf(stderr, "Can't get framebuffer info: %s\n", strerror(errno));
		return -1;
	}

	/* get framebuffer map address */
	void *guard1 = malloc(1);

	void *brkp = ec_sbrk(0);
	uintptr_t align = EC_ALIGN((uintptr_t)brkp, 0x1000) - (uintptr_t)brkp;
	ec_sbrk((intptr_t)align);

	fb_addr = ec_sbrk((intptr_t)fbinfo.size);
	if (!fb_addr) {

		fprintf(stderr, "Can't allocate region for framebuffer: %s\n", strerror(errno));
		return -1;
	}

	void *guard2 = malloc(1);

	/* map framebuffer */
	if (ec_ioctl(fd, ECIO_FB_MAP, (uintptr_t)&fb_addr) < 0) {

		fprintf(stderr, "Can't map framebuffer: %s\n", strerror(errno));
		return -1;
	}

	screen_image.width = (size_t)fbinfo.width;
	screen_image.height = (size_t)fbinfo.height;
	screen_image.depth_bytes = (size_t)fbinfo.depth_bytes;
	screen_image.pitch = (size_t)fbinfo.pitch;
	screen_image.data = fb_addr;

	/* draw initial cursor */
	curx = (int)fbinfo.width / 2;
	cury = (int)fbinfo.height / 2;
	ocurx = curx; ocury = cury;

	draw_cursor(curx, cury);
	return 0;
}

/* process event */
extern bool screen_process_event(wm_event_t *event) {

	switch (event->type) {
		/* move mouse cursor */
		case WM_EVENT_MOTION:
			ocurx = curx;
			ocury = cury;
			curx += event->motion.move.x;
			cury += event->motion.move.y;
			event->motion.position.x = curx;
			event->motion.position.y = cury;

			if (curx < 0) curx = 0;
			if (curx >= (int)fbinfo.width) curx = (int)fbinfo.width;
			if (cury < 0) cury = 0;
			if (cury >= (int)fbinfo.height) cury = (int)fbinfo.height;

			draw_cursor(ocurx, ocury);
			draw_cursor(curx, cury);
			break;
		/* mouse button */
		case WM_EVENT_BUTTON:
			event->button.position.x = curx;
			event->button.position.y = cury;
			break;
	}
	return true;
}

/* convert a color to screen format */
extern size_t screen_convert_color(uint8_t *buf, color_t color) {

	uint32_t pixel = 0;

	pixel |= ((uint32_t)color.r << fbinfo.rmask.size >> 8) << fbinfo.rmask.pos;
	pixel |= ((uint32_t)color.g << fbinfo.gmask.size >> 8) << fbinfo.gmask.pos;
	pixel |= ((uint32_t)color.b << fbinfo.bmask.size >> 8) << fbinfo.bmask.pos;

	for (uint32_t i = 0; i < fbinfo.depth_bytes; i++)
		buf[i] = (uint8_t)((pixel >> (i * 8)) & 0xff);
	return (size_t)fbinfo.depth_bytes;
}

/* clear screen */
extern void screen_clear(color_t color) {

	image_fill(&screen_image, color);
}

/* begin screen updates */
extern void screen_begin(void) {

	draw_cursor(curx, cury);
}

/* end screen updates */
extern void screen_end(void) {

	draw_cursor(curx, cury);
}
