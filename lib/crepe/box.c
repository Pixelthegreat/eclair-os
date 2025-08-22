/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crepe/core.h>
#include <crepe/box.h>

#define SPACING 4

/* box operations */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static void position(crepe_widget_t *widget);
static bool process_event(crepe_widget_t *widget, wm_event_t *event);
static void set_draw(crepe_widget_t *widget);
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void destroy(crepe_widget_t *widget);

crepe_widget_ops_t crepe_widget_ops_box = {
	.size = sizeof(crepe_box_t),
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

	crepe_box_t *box = CREPE_BOX(widget);

	size_t adjacent = 0, opposite = 0;
	for (size_t i = 0; i < box->nitems; i++) {

		crepe_widget_minimum_size(box->items[i], dc);
		switch (box->orientation) {

			/* horizontal */
			case CREPE_BOX_ORIENTATION_HORIZONTAL:
				adjacent += box->items[i]->mwidth + (i? SPACING: 0);
				opposite = CREPE_MAX(opposite, box->items[i]->mheight);
				break;

			/* vertical */
			case CREPE_BOX_ORIENTATION_VERTICAL:
				adjacent += box->items[i]->mheight + (i? SPACING: 0);
				opposite = CREPE_MAX(opposite, box->items[i]->mwidth);
				break;
		}
	}

	switch (box->orientation) {

		/* horizontal */
		case CREPE_BOX_ORIENTATION_HORIZONTAL:
			widget->mwidth = adjacent;
			widget->mheight = opposite;
			break;

		/* vertical */
		case CREPE_BOX_ORIENTATION_VERTICAL:
			widget->mwidth = opposite;
			widget->mheight = adjacent;
			break;
	}
}

/* calculate final size */
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	crepe_box_t *box = CREPE_BOX(widget);

	size_t adjacent = 0, opposite = 0;
	for (size_t i = 0; i < box->nitems; i++) {

		size_t spacing = (i? SPACING: 0);

		/* horizontal */
		if (box->orientation == CREPE_BOX_ORIENTATION_HORIZONTAL) {

			if (bwidth < box->items[i]->mwidth + spacing)
				break;

			crepe_widget_final_size(box->items[i], dc, bwidth, bheight);
			bwidth -= box->items[i]->width + spacing;

			adjacent += box->items[i]->width + spacing;
			opposite = CREPE_MAX(opposite, box->items[i]->height);
		}

		/* vertical */
		else {
			if (bheight < box->items[i]->mheight + spacing)
				break;

			crepe_widget_final_size(box->items[i], dc, bwidth, bheight);
			bheight -= box->items[i]->height + spacing;

			adjacent += box->items[i]->height + spacing;
			opposite = CREPE_MAX(opposite, box->items[i]->width);
		}
	}

	switch (box->orientation) {

		/* horizontal */
		case CREPE_BOX_ORIENTATION_HORIZONTAL:
			widget->width = adjacent;
			widget->height = opposite;
			break;

		/* vertical */
		case CREPE_BOX_ORIENTATION_VERTICAL:
			widget->width = opposite;
			widget->height = adjacent;
			break;
	}

	/* position widgets */
	int position = 0;
	for (size_t i = 0; i < box->nitems; i++) {

		/* horizontal */
		if (box->orientation == CREPE_BOX_ORIENTATION_HORIZONTAL) {

			box->items[i]->x = position;
			box->items[i]->y = (box->items[i]->valign * (int)(opposite - box->items[i]->height)) / 2;

			position += (int)box->items[i]->width + SPACING;
		}

		/* vertical */
		else {
			box->items[i]->x = (box->items[i]->halign * (int)(opposite - box->items[i]->width)) / 2;
			box->items[i]->y = position;

			position += (int)box->items[i]->height + SPACING;
		}
	}
}

/* calculate absolute position */
static void position(crepe_widget_t *widget) {

	crepe_box_t *box = CREPE_BOX(widget);

	for (size_t i = 0; i < box->nitems; i++)
		crepe_widget_position(box->items[i], widget);
}

/* process event */
static bool process_event(crepe_widget_t *widget, wm_event_t *event) {

	crepe_box_t *box = CREPE_BOX(widget);

	for (size_t i = 0; i < box->nitems; i++) {
		if (!crepe_widget_process_event(box->items[i], event))
			return false;
	}
	return true;
}

/* set needs to draw flag */
static void set_draw(crepe_widget_t *widget) {

	crepe_box_t *box = CREPE_BOX(widget);

	for (size_t i = 0; i < box->nitems; i++)
		crepe_widget_set_draw(box->items[i]);
}

/* draw widget */
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_box_t *box = CREPE_BOX(widget);

	for (size_t i = 0; i < box->nitems; i++)
		crepe_widget_draw(box->items[i], dc);
}

/* destroy widget */
static void destroy(crepe_widget_t *widget) {

	crepe_box_t *box = CREPE_BOX(widget);

	for (size_t i = 0; i < box->nitems; i++)
		crepe_widget_free(box->items[i]);
}

/* create box */
extern crepe_widget_t *crepe_box_new(crepe_box_orientation_t orientation) {

	crepe_box_t *box = CREPE_BOX(crepe_widget_new(&crepe_widget_ops_box));

	box->orientation = orientation;
	box->nitems = 0;
	memset(box->items, 0, sizeof(box->items));

	return CREPE_WIDGET(box);
}

/* add item to box */
extern void crepe_box_item(crepe_box_t *box, crepe_widget_t *item) {

	if (!box || !item) return;

	if (box->nitems >= CREPE_BOX_MAX_ITEMS)
		return;
	box->items[box->nitems++] = item;
}
