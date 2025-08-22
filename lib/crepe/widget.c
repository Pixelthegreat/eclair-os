/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <crepe/core.h>
#include <crepe/widget.h>

/* create widget */
extern crepe_widget_t *crepe_widget_new(crepe_widget_ops_t *ops) {

	crepe_widget_t *widget = (crepe_widget_t *)malloc(ops->size);

	widget->ops = ops;
	widget->x = 0;
	widget->y = 0;
	widget->absx = 0;
	widget->absy = 0;
	widget->mwidth = 0;
	widget->mheight = 0;
	widget->width = 0;
	widget->height = 0;
	widget->draw = true;
	widget->halign = CREPE_WIDGET_ALIGN_CENTER;
	widget->valign = CREPE_WIDGET_ALIGN_CENTER;
	widget->userdata = NULL;

	return widget;
}

/* calculate minimum size */
extern void crepe_widget_minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	if (!widget || !widget->ops->minimum_size)
		return;

	widget->ops->minimum_size(widget, dc);
}

/* calculate final size */
extern void crepe_widget_final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	if (!widget || !widget->ops->final_size)
		return;

	widget->ops->final_size(widget, dc, bwidth, bheight);
}

/* calculate absolute position */
extern void crepe_widget_position(crepe_widget_t *widget, crepe_widget_t *parent) {

	if (!widget) return;

	widget->absx = widget->x + (parent? parent->absx: 0);
	widget->absy = widget->y + (parent? parent->absy: 0);

	if (widget->ops->position)
		widget->ops->position(widget);
}

/* process event */
extern bool crepe_widget_process_event(crepe_widget_t *widget, wm_event_t *event) {

	if (!widget || !widget->ops->process_event)
		return true;

	return widget->ops->process_event(widget, event);
}

/* set needs to draw flag */
extern void crepe_widget_set_draw(crepe_widget_t *widget) {

	if (!widget || widget->draw) return;

	widget->draw = true;
	if (widget->ops->set_draw)
		widget->ops->set_draw(widget);
}

/* draw widget */
extern void crepe_widget_draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	if (!widget || !widget->ops->draw)
		return;

	widget->ops->draw(widget, dc);
	widget->draw = false;
}

/* free widget */
extern void crepe_widget_free(crepe_widget_t *widget) {

	if (!widget) return;

	if (widget->ops->destroy)
		widget->ops->destroy(widget);
	free(widget);
}
