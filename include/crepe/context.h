/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_CONTEXT_H
#define CREPE_CONTEXT_H

#include <crepe/core.h>
#include <crepe/draw.h>
#include <crepe/widget.h>
#include <ec/wm.h>

/* window widget */
struct crepe_context;

typedef struct crepe_window {
	crepe_widget_t base;
	struct crepe_context *context; /* associated context */
	struct crepe_window *previous; /* previous window in list */
	struct crepe_window *next; /* next window in list */
	crepe_widget_t *title; /* title widget */
	crepe_widget_t *widget; /* central widget */
	uint32_t window; /* window id */
	uint32_t image; /* image id */
	bool visible; /* wm visible state */
	size_t mwidth, mheight; /* default minimum width and height */
	bool close; /* destroy window */
} crepe_window_t;

#define CREPE_WINDOW(p) ((crepe_window_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_window;

/* context */
typedef struct crepe_context {
	bool init; /* initialized */
	crepe_window_t *first; /* first window in list */
	crepe_window_t *last; /* last window in list */
	crepe_draw_context_t *dc; /* drawing context */
	wm_screen_info_t scinfo; /* screen info */
} crepe_context_t;

#define CREPE_CONTEXT_INIT ((crepe_context_t){false})

/* functions */
extern crepe_widget_t *crepe_window_new(crepe_context_t *context, size_t mwidth, size_t mheight); /* create window */
extern void crepe_window_title(crepe_window_t *window, crepe_widget_t *title); /* set title bar widget */
extern void crepe_window_child(crepe_window_t *window, crepe_widget_t *widget); /* set central widget */
extern void crepe_window_present(crepe_window_t *window); /* present window contents */
extern void crepe_window_close(crepe_window_t *window); /* set window to close */

extern crepe_result_t crepe_context_init(crepe_context_t *context); /* initialize context */
extern void crepe_context_main_loop(crepe_context_t *context); /* main context loop */
extern void crepe_context_destroy(crepe_context_t *context); /* destroy context */

#endif /* CREPE_CONTEXT_H */
