/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <crepe/core.h>
#include <crepe/margin.h>

/* margin operations */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static void position(crepe_widget_t *widget);
static bool process_event(crepe_widget_t *widget, wm_event_t *event);
static void set_draw(crepe_widget_t *widget);
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void destroy(crepe_widget_t *widget);

crepe_widget_ops_t crepe_widget_ops_margin = {
	.size = sizeof(crepe_margin_t),
	.minimum_size = minimum_size,
	.final_size = final_size,
	.position = position,
	.process_event = process_event,
	.set_draw = set_draw,
	.draw = draw,
	.destroy = destroy,
};

/* calculate minimum size */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_margin_t *margin = CREPE_MARGIN(widget);

	widget->mwidth = margin->left + margin->right;
	widget->mheight = margin->top + margin->bottom;

	if (margin->widget) {
		
		crepe_widget_minimum_size(margin->widget, dc);

		widget->mwidth += margin->widget->mwidth;
		widget->mheight += margin->widget->mheight;
	}
}

/* calculate final size */
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	crepe_margin_t *margin = CREPE_MARGIN(widget);

	bwidth -= margin->left + margin->right;
	bheight -= margin->top + margin->bottom;

	widget->width = margin->left + margin->right;
	widget->height = margin->top + margin->bottom;

	if (margin->widget) {

		crepe_widget_final_size(margin->widget, dc, bwidth, bheight);

		widget->width += margin->widget->width;
		widget->height += margin->widget->height;

		margin->widget->x = (int)margin->left;
		margin->widget->y = (int)margin->top;
	}
}

/* calculate absolute position */
static void position(crepe_widget_t *widget) {

	crepe_margin_t *margin = CREPE_MARGIN(widget);
	crepe_widget_position(margin->widget, widget);
}

/* process event */
static bool process_event(crepe_widget_t *widget, wm_event_t *event) {

	crepe_margin_t *margin = CREPE_MARGIN(widget);
	crepe_widget_process_event(margin->widget, event);
}

/* set needs to draw flag */
static void set_draw(crepe_widget_t *widget) {

	crepe_margin_t *margin = CREPE_MARGIN(widget);
	crepe_widget_set_draw(margin->widget);
}

/* draw widget */
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_margin_t *margin = CREPE_MARGIN(widget);
	crepe_widget_draw(margin->widget, dc);
}

/* destroy widget */
static void destroy(crepe_widget_t *widget) {

	crepe_margin_t *margin = CREPE_MARGIN(widget);
	crepe_widget_free(margin->widget);
}

/* create margin area */
extern crepe_widget_t *crepe_margin_new(size_t left, size_t right, size_t top, size_t bottom) {

	crepe_margin_t *margin = CREPE_MARGIN(crepe_widget_new(&crepe_widget_ops_margin));

	margin->left = left;
	margin->right = right;
	margin->top = top;
	margin->bottom = bottom;
	margin->widget = NULL;

	return CREPE_WIDGET(margin);
}

/* set child */
extern void crepe_margin_child(crepe_margin_t *margin, crepe_widget_t *widget) {

	margin->widget = widget;
}
