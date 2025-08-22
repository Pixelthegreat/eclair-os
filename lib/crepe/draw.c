/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ec/image.h>
#include <crepe/core.h>
#include <crepe/draw.h>

static int fontsizes[CREPE_TEXT_STYLE_COUNT * 2] = {
	6, 11, /* normal */
	8, 11, /* bold */
};

/* load image from file */
#define MAX_COLORS WM_SET_IMAGE_DATA_MAX(WM_FORMAT_RGB8)
static uint8_t imgdata[MAX_COLORS * 3];

extern uint32_t crepe_load_image(const char *path, uint32_t *width, uint32_t *height) {

	uint32_t twidth, theight;
	if (!width) width = &twidth;
	if (!height) height = &theight;

	ec_image_t image = EC_IMAGE_INIT;
	if (ec_image_open(&image, path, EC_IMAGE_FORMAT_RBN) < 0)
		return WM_NULL;

	*width = (uint32_t)image.width;
	*height = (uint32_t)image.height;

	/* load image to window manager */
	uint32_t handle = wm_create_image(*width, *height, WM_CLASS_IMAGE);
	if (!handle) {

		ec_image_close(&image);
		return WM_NULL;
	}

	size_t position = 0;
	size_t size = image.width * image.height;

	while (position < size) {

		size_t toread = size - position < MAX_COLORS? size - position: MAX_COLORS;

		if (ec_image_read_colors(&image, imgdata, toread, EC_IMAGE_DATA_RGB8) < 0 ||
		    wm_set_image_data(handle, WM_FORMAT_RGB8, (uint32_t)position, (uint32_t)toread, imgdata) < 0) {

			ec_image_close(&image);
			wm_destroy_image(handle);
			return WM_NULL;
		}
		position += toread;
	}
	ec_image_close(&image);
	return handle;
}

/* create drawing context */
extern crepe_draw_context_t *crepe_draw_context_new(void) {

	crepe_draw_context_t *context = (crepe_draw_context_t *)malloc(sizeof(crepe_draw_context_t));

	context->image = 0;
	context->width = 0;
	context->height = 0;
	context->ncommands = 0;
	context->state.x = 0;
	context->state.y = 0;
	memset(context->fonts, 0, sizeof(context->fonts));
	context->ui = 0;

	/* load font faces */
	for (size_t i = 0; i < CREPE_TEXT_STYLE_COUNT; i++) {

		char buf[64];
		snprintf(buf, 64, "/usr/share/font11%c.rbn", (char)i + 'a');
		context->fonts[i] = crepe_load_image(buf, NULL, NULL);
		if (!context->fonts[i]) {

			crepe_draw_context_free(context);
			return NULL;
		}
	}

	/* load ui image */
	context->ui = crepe_load_image("/usr/share/ui.rbn", NULL, NULL);
	if (!context->ui) {

		crepe_draw_context_free(context);
		return NULL;
	}
	return context;
}

/* set destination and drawing area */
extern void crepe_draw_context_destination(crepe_draw_context_t *context, uint32_t image, uint32_t width, uint32_t height) {

	context->image = image;
	context->width = width;
	context->height = height;
}

/* begin drawing pass */
extern void crepe_draw_context_begin(crepe_draw_context_t *context) {

	context->ncommands = 0;
	context->state.x = 0;
	context->state.y = 0;
}

/* clear drawing area */
extern void crepe_draw_context_clear(crepe_draw_context_t *context, crepe_color_t color) {

	size_t index = context->ncommands++;

	context->commands[index].type = WM_DRAW_FILL;
	context->commands[index].fill.color[0] = color.r;
	context->commands[index].fill.color[1] = color.g;
	context->commands[index].fill.color[2] = color.b;

	if (context->ncommands >= WM_DRAW_BATCH_MAX)
		crepe_draw_context_flush(context);
}

/* fill rect with color */
extern void crepe_draw_context_fill_rect(crepe_draw_context_t *context, size_t w, size_t h, crepe_color_t color) {

	size_t index = context->ncommands++;

	context->commands[index].type = WM_DRAW_FILL_RECT;
	context->commands[index].fill_rect.x = context->state.x;
	context->commands[index].fill_rect.y = context->state.y;
	context->commands[index].fill_rect.w = (uint32_t)w;
	context->commands[index].fill_rect.h = (uint32_t)h;
	context->commands[index].fill_rect.color[0] = color.r;
	context->commands[index].fill_rect.color[1] = color.g;
	context->commands[index].fill_rect.color[2] = color.b;
}

/* set position */
extern void crepe_draw_context_position(crepe_draw_context_t *context, int x, int y) {

	context->state.x = x;
	context->state.y = y;
}

/* calculate text size */
extern void crepe_draw_context_text_size(crepe_draw_context_t *context, crepe_text_style_t style, const char *text, int *w, int *h) {

	crepe_text_style_t flags = style & CREPE_TEXT_STYLE_FLAGS;
	style &= CREPE_TEXT_STYLE;

	int cw = fontsizes[style * 2];
	int ch = fontsizes[style * 2 + 1];

	*w = 0;
	*h = ch;
	int mw = 0;

	while (*text) {

		char c = *text++;
		if (c == '\n') {

			if (mw > *w) *w = mw;
			mw = 0;
			*h += ch;
		}
		else if (c >= ' ' && c <= '~') mw += cw;
	}
	if (mw > *w) *w = mw;
}

/* draw text */
extern void crepe_draw_context_text(crepe_draw_context_t *context, crepe_text_style_t style, const char *text) {

	int x = context->state.x;
	int y = context->state.y;

	crepe_text_style_t flags = style & CREPE_TEXT_STYLE_FLAGS;
	style &= CREPE_TEXT_STYLE;

	int cw = fontsizes[style * 2];
	int ch = fontsizes[style * 2 + 1];
	int offset = (flags & CREPE_TEXT_STYLE_INVERTED)? ch * 6: 0;

	while (*text) {

		char c = *text++;
		if (c == '\n') {

			x = context->state.x;
			y += ch;
			continue;
		}
		else if (c < ' ' || c > '~') continue;

		size_t index = context->ncommands++;

		context->commands[index].type = WM_DRAW_COPY_AREA;
		context->commands[index].copy_area.id = context->fonts[style];
		context->commands[index].copy_area.x = x;
		context->commands[index].copy_area.y = y;
		context->commands[index].copy_area.sy = (((int32_t)c - ' ') / 16) * ch + offset;
		context->commands[index].copy_area.sx = (((int32_t)c - ' ') % 16) * cw;
		context->commands[index].copy_area.sw = (uint32_t)cw;
		context->commands[index].copy_area.sh = (uint32_t)ch;

		if (context->ncommands >= WM_DRAW_BATCH_MAX)
			crepe_draw_context_flush(context);

		x += cw;
	}
}

/* draw image */
extern void crepe_draw_context_image(crepe_draw_context_t *context, uint32_t image, int x, int y, size_t w, size_t h) {

	size_t index = context->ncommands++;

	context->commands[index].type = WM_DRAW_COPY_AREA;
	context->commands[index].copy_area.id = image;
	context->commands[index].copy_area.x = context->state.x;
	context->commands[index].copy_area.y = context->state.y;
	context->commands[index].copy_area.sx = x;
	context->commands[index].copy_area.sy = y;
	context->commands[index].copy_area.sw = (uint32_t)w;
	context->commands[index].copy_area.sh = (uint32_t)h;

	if (context->ncommands >= WM_DRAW_BATCH_MAX)
		crepe_draw_context_flush(context);
}

/* draw image repeated */
extern void crepe_draw_context_image_repeated(crepe_draw_context_t *context, uint32_t image, int x, int y, size_t sw, size_t sh, size_t w, size_t h) {

	int ax = context->state.x;
	int ay = context->state.y;

	while (h > 0) {

		size_t ph = h > sh? sh: h;
		size_t ow = w;
		while (w > 0) {

			size_t pw = w > sw? sw: w;

			size_t index = context->ncommands++;

			context->commands[index].type = WM_DRAW_COPY_AREA;
			context->commands[index].copy_area.id = image;
			context->commands[index].copy_area.x = ax;
			context->commands[index].copy_area.y = ay;
			context->commands[index].copy_area.sx = x;
			context->commands[index].copy_area.sy = y;
			context->commands[index].copy_area.sw = (uint32_t)pw;
			context->commands[index].copy_area.sh = (uint32_t)ph;

			if (context->ncommands >= WM_DRAW_BATCH_MAX)
				crepe_draw_context_flush(context);

			w -= pw;
			ax += (int)pw;
		}
		w = ow;
		ax = context->state.x;

		h -= ph;
		ay += (int)ph;
	}
}

/* draw border helper */
extern void crepe_draw_context_border(crepe_draw_context_t *context, uint32_t image, int x, int y, size_t sw, size_t sh, size_t w, size_t h, size_t bw) {

	int ax = context->state.x;
	int ay = context->state.y;

	/* top-left corner */
	crepe_draw_context_position(context, ax, ay);
	crepe_draw_context_image(context, image, x, y, bw, bw);

	/* top edge */
	crepe_draw_context_position(context, ax+(int)bw, ay);
	crepe_draw_context_image_repeated(context, image, x+(int)bw, y, sw-bw*2, bw, w-bw*2, bw);

	/* top-right corner */
	crepe_draw_context_position(context, ax+(int)(w-bw), ay);
	crepe_draw_context_image(context, image, x+(int)(sw-bw), y, bw, bw);

	/* right edge */
	crepe_draw_context_position(context, ax+(int)(w-bw), ay+bw);
	crepe_draw_context_image_repeated(context, image, x+(int)(sw-bw), y+(int)bw, bw, sh-bw*2, bw, h-bw*2);

	/* bottom-right corner */
	crepe_draw_context_position(context, ax+(int)(w-bw), ay+(int)(h-bw));
	crepe_draw_context_image(context, image, x+(int)(sw-bw), y+(int)(sh-bw), bw, bw);

	/* bottom edge */
	crepe_draw_context_position(context, ax+(int)bw, ay+(int)(h-bw));
	crepe_draw_context_image_repeated(context, image, x+(int)bw, y+(int)(sh-bw), sw-bw*2, bw, (int)(w-bw*2), (int)bw);

	/* bottom-left corner */
	crepe_draw_context_position(context, ax, ay+(int)(h-bw));
	crepe_draw_context_image(context, image, x, y+(int)(sh-bw), bw, bw);

	/* left edge */
	crepe_draw_context_position(context, ax, ay+(int)bw);
	crepe_draw_context_image_repeated(context, image, x, y+(int)bw, bw, sh-bw*2, bw, h-bw*2);

	crepe_draw_context_position(context, ax, ay);
}

/* flush drawing commands */
extern void crepe_draw_context_flush(crepe_draw_context_t *context) {

	if (!context->ncommands) return;

	wm_draw_batch(context->image, (uint32_t)context->ncommands, context->commands);
	context->ncommands = 0;
}

/* end drawing pass */
extern void crepe_draw_context_end(crepe_draw_context_t *context) {

	crepe_draw_context_flush(context);
}

/* free drawing context */
extern void crepe_draw_context_free(crepe_draw_context_t *context) {

	if (!context) return;

	if (context->ui) wm_destroy_image(context->ui);
	for (size_t i = 0; i < CREPE_TEXT_STYLE_COUNT; i++) {

		if (context->fonts[i])
			wm_destroy_image(context->fonts[i]);
	}

	free(context);
}
