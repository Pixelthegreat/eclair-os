/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec/device.h>
#include <ec/keycode.h>
#include <ec.h>

static int fd = -1; /* framebuffer file descriptor */
static int mfd = -1; /* mouse file descriptor */
static void *fb_addr = NULL; /* framebuffer address */
static int mx, my; /* mouse position */
static int ox, oy; /* old mouse position */

#define CURSOR_HEIGHT 10
static uint8_t cursor[CURSOR_HEIGHT] = {
	0b00000001,
	0b00000011,
	0b00000111,
	0b00001111,
	0b00011111,
	0b00111111,
	0b01111111,
	0b00001111,
	0b00011001,
	0b00010000,
};

/* flip pixel */
static void flippixel(ecio_fbinfo_t *fbinfo, int x, int y) {

	if (x >= fbinfo->width || y >= fbinfo->height) return;

	uint8_t *addr = (uint8_t *)(fb_addr + (uint32_t)y * fbinfo->pitch + (uint32_t)x * fbinfo->depth_bytes);

	for (uint32_t i = 0; i < fbinfo->depth_bytes; i++)
		addr[i] ^= 0xff;
}

/* draw mouse */
static void drawmouse(ecio_fbinfo_t *fbinfo, bool old, bool new) {

	if (old) {

		for (int y = 0; y < CURSOR_HEIGHT; y++) {
			for (int x = 0; x < 8; x++) {
				if (cursor[y] & (1 << x))
					flippixel(fbinfo, ox+x, oy+y);
			}
		}
	}
	if (new) {

		for (int y = 0; y < CURSOR_HEIGHT; y++) {
			for (int x = 0; x < 8; x++) {
				if (cursor[y] & (1 << x))
					flippixel(fbinfo, mx+x, my+y);
			}
		}
	}
}

/* run application */
static int run(void) {

	fd = ec_open("/dev/fb0", ECF_READ, 0);
	if (fd < 0) {

		fprintf(stderr, "Failed to open '/dev/fb0': %s\n", strerror(errno));
		return 1;
	}

	mfd = ec_open("/dev/mus0", ECF_READ, 0);
	if (mfd < 0) {

		fprintf(stderr, "Failed to open '/dev/mus0': %s\n", strerror(errno));
		return 1;
	}

	ecio_fbinfo_t fbinfo;
	if (ec_ioctl(fd, ECIO_FB_GETINFO, (uintptr_t)&fbinfo) < 0) {

		fprintf(stderr, "Failed to get framebuffer info: %s\n", strerror(errno));
		return 1;
	}

	/* get address to map framebuffer to */
	void *guard1 = malloc(1); /* guard allocation to prevent malloc from interfering with the framebuffer region */

	void *brkp = ec_sbrk(0);
	uintptr_t align = EC_ALIGN((uintptr_t)brkp, 0x1000) - (uintptr_t)brkp;

	ec_sbrk((intptr_t)align);
	fb_addr = ec_sbrk((intptr_t)fbinfo.size);
	if (!fb_addr) {

		fprintf(stderr, "Failed to allocate region for framebuffer: %s\n", strerror(errno));
		return 1;
	}

	void *guard2 = malloc(1);

	/* map framebuffer */
	if (ec_ioctl(fd, ECIO_FB_MAP, (uintptr_t)&fb_addr) < 0) {

		fprintf(stderr, "Failed to map framebuffer: %s\n", strerror(errno));
		return 1;
	}

	/* initialize mouse cursor */
	mx = (int)fbinfo.width / 2;
	my = (int)fbinfo.height / 2;
	ox = mx;
	oy = my;
	drawmouse(&fbinfo, false, true);

	ec_ioctl(mfd, ECIO_INP_FLUSH, 0);

	/* loop */
	ec_msevent_t event;
	bool running = true;
	while (running) {

		while (!ec_ioctl(mfd, ECIO_INP_GETEVENT, (uintptr_t)&event)) {

			ox = mx;
			oy = my;
			mx += event.x;
			my += event.y;

			if (mx < 0) mx = 0;
			else if (mx >= (int)fbinfo.width) mx = (int)fbinfo.width;
			if (my < 0) my = 0;
			else if (my >= (int)fbinfo.height) my = (int)fbinfo.height;

			drawmouse(&fbinfo, true, true);

			if (event.state[ECB_LEFT]) running = false;
		}

		ec_timeval_t tv = {.sec = 0, .nsec = 16666667};
		ec_sleepns(&tv);
	}
	drawmouse(&fbinfo, true, false);
	return 0;
}

/* clean up resources */
static void cleanup(void) {

	if (mfd >= 0) ec_close(mfd);
	if (fd >= 0) ec_close(fd);
}

int main() {

	int code = run();
	cleanup();
	return code;
}
