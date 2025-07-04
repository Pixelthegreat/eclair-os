#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec/device.h>
#include <ec.h>

static int fd = -1; /* framebuffer file descriptor */
static void *fb_addr = NULL; /* framebuffer address */
static int cursor = 1; /* cursor state */

/* flip cursor state */
static int flipcursor(void) {

	if (ec_ioctl(0, ECIO_TTY_CURSOR, 1) < 0) {

		fprintf(stderr, "Failed to flip cursor blink state: %s\n", strerror(errno));
		return -1;
	}
	cursor = !cursor;
	return 0;
}

/* set pixel in framebuffer (same code as in kernel/driver/fb.c) */
struct color {
	uint8_t r, g, b;
};

static void setpixel(ecio_fbinfo_t *fbinfo, uint32_t x, uint32_t y, struct color c) {

	if (x >= fbinfo->width || y >= fbinfo->height) return;

	uint8_t *addr = (uint8_t *)(fb_addr + y * fbinfo->pitch + x * fbinfo->depth_bytes);
	uint32_t pixel = 0;

	pixel |= ((uint32_t)c.r * fbinfo->rmask.size / 8) << fbinfo->rmask.pos;
	pixel |= ((uint32_t)c.g * fbinfo->gmask.size / 8) << fbinfo->gmask.pos;
	pixel |= ((uint32_t)c.b * fbinfo->bmask.size / 8) << fbinfo->bmask.pos;

	for (uint32_t i = 0; i < fbinfo->depth_bytes; i++)
		addr[i] = (uint8_t)((pixel >> (i * 8)) & 0xff);
}

/* run application */
static int run(void) {

	(void)flipcursor();

	fd = ec_open("/dev/fb0", ECF_READ, 0);
	if (fd < 0) {

		fprintf(stderr, "Failed to open 'dev/fb0': %s\n", strerror(errno));
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

		fprintf(stderr, "Failed to map framebuffer; %s\n", strerror(errno));
		return 1;
	}

	/* draw picture */
	for (uint32_t y = 0; y < fbinfo.height; y++) {
		for (uint32_t x = 0; x < fbinfo.width * fbinfo.depth_bytes; x++) {

			struct color color = {
				x & 0xff,
				0xff - (x & 0xff),
				y & 0xff,
			};
			setpixel(&fbinfo, x, y, color);
		}
	}

	/* wait and clear screen */
	ec_timeval_t tv = {
		.sec = 3,
		.nsec = 0,
	};
	ec_sleepns(&tv);

	struct color clear = {0, 0, 0};
	for (uint32_t y = 0; y < fbinfo.height; y++) {
		for (uint32_t x = 0; x < fbinfo.width * fbinfo.depth_bytes; x++)
			setpixel(&fbinfo, x, y, clear);
	}

	return 0;
}

/* clean up resources */
static void cleanup(void) {

	if (fd >= 0) ec_close(fd);
	if (!cursor) (void)flipcursor();
}

int main() {

	int code = run();
	cleanup();
	return code;
}
