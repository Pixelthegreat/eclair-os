/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_WIDGET_H
#define CREPE_WIDGET_H

#include <ec/wm.h>
#include <crepe/core.h>
#include <crepe/draw.h>

/* widget operations and type info */
struct crepe_widget;

typedef struct crepe_widget_ops {
	size_t size; /* widget size */
	void (*minimum_size)(struct crepe_widget *, crepe_draw_context_t *); /* calculate minimum size */
	void (*final_size)(struct crepe_widget *, crepe_draw_context_t *, size_t, size_t); /* calculate final size */
	void (*position)(struct crepe_widget *); /* calculate absolute position */
	bool (*process_event)(struct crepe_widget *, wm_event_t *); /* process event */
	void (*set_draw)(struct crepe_widget *); /* set needs to draw flag */
	void (*draw)(struct crepe_widget *, crepe_draw_context_t *); /* draw widget */
	void (*destroy)(struct crepe_widget *); /* clean up resources */
} crepe_widget_ops_t;

/* widget alignments */
typedef enum crepe_widget_align {
	CREPE_WIDGET_ALIGN_START = 0,
	CREPE_WIDGET_ALIGN_CENTER,
	CREPE_WIDGET_ALIGN_END,

	CREPE_WIDGET_ALIGN_COUNT,
} crepe_widget_align_t;

/* widget */
typedef struct crepe_widget {
	crepe_widget_ops_t *ops; /* operations */
	int x, y; /* relative position */
	int absx, absy; /* absolute position */
	size_t mwidth, mheight; /* minimum size */
	size_t width, height; /* final size */
	bool draw; /* needs to draw flag */
	crepe_widget_align_t halign; /* norizontal align */
	crepe_widget_align_t valign; /* vertical align */
	void *userdata; /* user data */
} crepe_widget_t;

#define CREPE_WIDGET(p) ((crepe_widget_t *)(p))

/* functions */
extern crepe_widget_t *crepe_widget_new(crepe_widget_ops_t *ops); /* create widget */
extern void crepe_widget_minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc); /* calculate minimum size */
extern void crepe_widget_final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight); /* calculate final size */
extern void crepe_widget_position(crepe_widget_t *widget, crepe_widget_t *parent); /* calculate absolute position */
extern bool crepe_widget_process_event(crepe_widget_t *widget, wm_event_t *event); /* process event */
extern void crepe_widget_set_draw(crepe_widget_t *widget); /* set needs to draw flag */
extern void crepe_widget_draw(crepe_widget_t *widget, crepe_draw_context_t *dc); /* draw widget */
extern void crepe_widget_free(crepe_widget_t *widget); /* free widget */

#endif /* CREPE_WIDGET_H */
