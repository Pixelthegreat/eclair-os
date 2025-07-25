/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WM_SCREEN_H
#define WM_SCREEN_H

#include <stddef.h>
#include <stdint.h>
#include <ec/wm.h>

/* color */
typedef struct color {
	uint8_t r, g, b; /* components */
} color_t;

/* image */
typedef struct image {
	size_t width, height; /* size */
	size_t depth_bytes; /* depth in bytes */
	size_t pitch; /* bytes between lines */
	void *data; /* image data */
} image_t;

#define IMAGE_INIT ((image_t){.data = NULL})

extern image_t screen_image; /* main screen image */

/* functions */
extern void image_init(image_t *image, size_t width, size_t height); /* initialize image */
extern void image_fill(image_t *image, color_t color); /* fill image with color */
extern void image_copy_area(image_t *dest, image_t *source, int x, int y, int sx, int sy, int sw, int sh); /* copy image area */
extern void image_resize(image_t *image, size_t width, size_t height); /* resize image */
extern void image_destroy(image_t *image); /* destroy image */

extern int screen_init(void); /* initialize screen */
extern bool screen_process_event(wm_event_t *event); /* process event */
extern size_t screen_convert_color(uint8_t *buf, color_t color); /* convert a color to screen format */
extern void screen_clear(color_t color); /* clear screen */
extern void screen_begin(void); /* begin screen updates */
extern void screen_end(void); /* end screen updates */

#endif /* WM_SCREEN_H */
