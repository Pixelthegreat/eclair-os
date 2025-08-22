/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <crepe/core.h>
#include <crepe/label.h>

/* label operations */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc);

crepe_widget_ops_t crepe_widget_ops_label = {
	.size = sizeof(crepe_label_t),
	.minimum_size = minimum_size,
	.final_size = final_size,
	.draw = draw,
};

/* calculate minimum size */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_label_t *label = CREPE_LABEL(widget);

	int w, h;
	crepe_draw_context_text_size(dc, label->style, label->text, &w, &h);

	widget->mwidth = (size_t)w;
	widget->mheight = (size_t)h;
}

/* calculate final size */
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	widget->width = widget->mwidth;
	widget->height = widget->mheight;
}

/* draw label */
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_label_t *label = CREPE_LABEL(widget);
	if (widget->draw) {

		crepe_draw_context_position(dc, widget->absx, widget->absy);
		crepe_draw_context_text(dc, label->style, label->text);
	}
}

/* create label */
extern crepe_widget_t *crepe_label_new(crepe_text_style_t style, const char *text) {

	crepe_label_t *label = CREPE_LABEL(crepe_widget_new(&crepe_widget_ops_label));

	label->style = style;
	label->text = text;

	return CREPE_WIDGET(label);
}
