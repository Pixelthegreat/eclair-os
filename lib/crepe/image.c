/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <crepe/core.h>
#include <crepe/draw.h>
#include <crepe/image.h>

/* image operations */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static void set_draw(crepe_widget_t *widget);
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void destroy(crepe_widget_t *widget);

crepe_widget_ops_t crepe_widget_ops_image = {
	.size = sizeof(crepe_image_t),
	.minimum_size = minimum_size,
	.final_size = final_size,
	.set_draw = set_draw,
	.draw = draw,
	.destroy = destroy,
};

/* calculate minimum size */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_image_t *image = CREPE_IMAGE(widget);

	widget->mwidth = image->width;
	widget->mheight = image->height;
}

/* calculate final size */
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	crepe_image_t *image = CREPE_IMAGE(widget);

	widget->width = image->width;
	widget->height = image->height;
}

/* set needs to draw flag (stub) */
static void set_draw(crepe_widget_t *widget) {
}

/* draw widget */
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_image_t *image = CREPE_IMAGE(widget);
	if (widget->draw) {

		crepe_draw_context_position(dc, widget->absx, widget->absy);
		crepe_draw_context_image(dc, image->image, 0, 0, image->width, image->height);
	}
}

/* destroy */
static void destroy(crepe_widget_t *widget) {

	crepe_image_t *image = CREPE_IMAGE(widget);
	wm_destroy_image(image->image);
}

/* create image from file */
extern crepe_widget_t *crepe_image_new_from_file(const char *path) {

	uint32_t w, h;
	uint32_t handle = crepe_load_image(path, &w, &h);

	if (!handle) return NULL;

	crepe_image_t *image = CREPE_IMAGE(crepe_widget_new(&crepe_widget_ops_image));

	image->image = handle;
	image->width = (size_t)w;
	image->height = (size_t)h;

	return CREPE_WIDGET(image);
}
