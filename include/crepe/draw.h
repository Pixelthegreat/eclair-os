/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_DRAW_H
#define CREPE_DRAW_H

#include <ec/wm.h>
#include <crepe/core.h>

/* color */
typedef struct crepe_color {
	uint8_t r, g, b; /* color components */
} crepe_color_t;

#define CREPE_COLOR(r, g, b) ((crepe_color_t){r, g, b})

/* text styles */
typedef enum crepe_text_style {
	CREPE_TEXT_STYLE_NORMAL = 0,
	CREPE_TEXT_STYLE_BOLD,

	CREPE_TEXT_STYLE_COUNT,

	CREPE_TEXT_STYLE = 0xf,
	CREPE_TEXT_STYLE_FLAGS = ~0xf,

	CREPE_TEXT_STYLE_INVERTED = 0x10,
} crepe_text_style_t;

/* context */
typedef struct crepe_draw_context {
	uint32_t image; /* destination image */
	uint32_t width, height; /* drawing area */
	wm_draw_command_t commands[WM_DRAW_BATCH_MAX]; /* drawing commands */
	size_t ncommands; /* number of commands */
	struct {
		int x, y; /* position */
	} state; /* drawing state */
	uint32_t fonts[CREPE_TEXT_STYLE_COUNT]; /* font images */
	uint32_t ui; /* ui image */
} crepe_draw_context_t;

/* functions */
extern uint32_t crepe_load_image(const char *path, uint32_t *width, uint32_t *height); /* load image from file */

extern crepe_draw_context_t *crepe_draw_context_new(void); /* create drawing context */
extern void crepe_draw_context_destination(crepe_draw_context_t *context, uint32_t image, uint32_t width, uint32_t height); /* set destination and drawing area */
extern void crepe_draw_context_begin(crepe_draw_context_t *context); /* begin drawing pass */
extern void crepe_draw_context_clear(crepe_draw_context_t *context, crepe_color_t color); /* clear drawing area */
extern void crepe_draw_context_fill_rect(crepe_draw_context_t *context, size_t w, size_t h, crepe_color_t color); /* fill rect with color */
extern void crepe_draw_context_position(crepe_draw_context_t *context, int x, int y); /* set position */
extern void crepe_draw_context_text_size(crepe_draw_context_t *context, crepe_text_style_t style, const char *text, int *w, int *h); /* calculate text size */
extern void crepe_draw_context_text(crepe_draw_context_t *context, crepe_text_style_t style, const char *text); /* draw text */
extern void crepe_draw_context_image(crepe_draw_context_t *context, uint32_t image, int x, int y, size_t w, size_t h); /* draw image */
extern void crepe_draw_context_image_repeated(crepe_draw_context_t *context, uint32_t image, int x, int y, size_t sw, size_t sh, size_t w, size_t h); /* draw image repeated */
extern void crepe_draw_context_border(crepe_draw_context_t *context, uint32_t image, int x, int y, size_t sw, size_t sh, size_t w, size_t h, size_t bw); /* draw border helper */
extern void crepe_draw_context_flush(crepe_draw_context_t *context); /* flush drawing commands */
extern void crepe_draw_context_end(crepe_draw_context_t *context); /* end drawing pass */
extern void crepe_draw_context_free(crepe_draw_context_t *context); /* free drawing context */

#endif /* CREPE_DRAW_H */
