/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <ec.h>
#include <ec/wm.h>
#include <crepe/core.h>
#include <crepe/title.h>
#include <crepe/context.h>

#define FPS 20

#define TITLE_BORDER_WIDTH 3
#define CHILD_BORDER_START 4
#define CHILD_BORDER_WIDTH 5

static const int outer[4] = {
	0, 0, 256, 256, /* outer border */
};
static const int inner[4] = {
	3, 21, 250, 232, /* inner border */
};

/* window operations */
static void window_minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void window_final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static void window_position(crepe_widget_t *widget);
static bool window_process_event(crepe_widget_t *widget, wm_event_t *event);
static void window_set_draw(crepe_widget_t *widget);
static void window_draw(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void window_destroy(crepe_widget_t *widget);

crepe_widget_ops_t crepe_widget_ops_window = {
	.size = sizeof(crepe_window_t),
	.minimum_size = window_minimum_size,
	.final_size = window_final_size,
	.position = window_position,
	.process_event = window_process_event,
	.set_draw = window_set_draw,
	.draw = window_draw,
	.destroy = window_destroy,
};

/* calculate minimum size */
static void window_minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_window_t *window = CREPE_WINDOW(widget);

	size_t width = 0;
	size_t height = 0;

	if (window->title) {

		crepe_widget_minimum_size(window->title, dc);
		width = CREPE_MAX(width, window->title->mwidth);

		height += window->title->mheight;
	}

	if (window->widget) {
		
		crepe_widget_minimum_size(window->widget, dc);
		width = CREPE_MAX(width, window->widget->mwidth + (CHILD_BORDER_WIDTH - TITLE_BORDER_WIDTH));
		height += window->widget->mheight + (CHILD_BORDER_WIDTH - TITLE_BORDER_WIDTH);
	}
	width += TITLE_BORDER_WIDTH*2;
	height += CHILD_BORDER_WIDTH + (window->title? CHILD_BORDER_START: TITLE_BORDER_WIDTH);

	widget->mwidth = CREPE_MAX(width, window->mwidth);
	widget->mheight = CREPE_MAX(height, window->mheight);
}

/* calculate final size */
static void window_final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	crepe_window_t *window = CREPE_WINDOW(widget);

	if (window->title)
		crepe_widget_final_size(window->title, dc, widget->mwidth - TITLE_BORDER_WIDTH*2, widget->mheight - TITLE_BORDER_WIDTH*2);
	size_t theight = window->title? window->title->height: 0;
	size_t tstart = window->title? CHILD_BORDER_START: TITLE_BORDER_WIDTH;

	bwidth = widget->mwidth - CHILD_BORDER_START*2;
	bheight = widget->mheight - CHILD_BORDER_START - tstart - 1 - theight;
	if (window->widget)
		crepe_widget_final_size(window->widget, dc, bwidth, bheight);

	widget->width = widget->mwidth;
	widget->height = widget->mheight;

	/* position title and child */
	if (window->title) {

		window->title->x = TITLE_BORDER_WIDTH;
		window->title->y = TITLE_BORDER_WIDTH;
	}

	if (window->widget) {

		window->widget->x = CHILD_BORDER_START + (window->widget->halign * (int)(bwidth - window->widget->width)) / 2;
		window->widget->y = (int)tstart + 1 + (int)theight + (window->widget->valign * (int)(bheight - window->widget->height)) / 2;
	}
}

/* calculate absolute position */
static void window_position(crepe_widget_t *widget) {

	crepe_window_t *window = CREPE_WINDOW(widget);
	crepe_widget_position(window->title, widget);
	crepe_widget_position(window->widget, widget);
}

/* process event */
static bool window_process_event(crepe_widget_t *widget, wm_event_t *event) {

	crepe_window_t *window = CREPE_WINDOW(widget);

	if (!crepe_widget_process_event(window->title, event))
		return false;
	if (!crepe_widget_process_event(window->widget, event))
		return false;
	return true;
}

/* set needs to draw flag */
static void window_set_draw(crepe_widget_t *widget) {

	crepe_window_t *window = CREPE_WINDOW(widget);
	crepe_widget_set_draw(window->title);
	crepe_widget_set_draw(window->widget);
}

/* draw window */
static void window_draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_window_t *window = CREPE_WINDOW(widget);
	if (!window->image) return;

	crepe_draw_context_destination(dc, window->image, widget->width, widget->height);
	crepe_draw_context_begin(dc);

	if (widget->draw) {

		size_t theight = window->title? window->title->height: 0;
		size_t tstart = window->title? CHILD_BORDER_START: TITLE_BORDER_WIDTH;

		/* draw border */
		crepe_draw_context_clear(dc, CREPE_COLOR(0xb9, 0xbe, 0xd4));
		crepe_draw_context_position(dc, 0, 0);
		crepe_draw_context_border(dc, dc->ui, outer[0], outer[1], (size_t)outer[2], (size_t)outer[3], widget->width, widget->height, 2);
		crepe_draw_context_position(dc, TITLE_BORDER_WIDTH, (int)tstart + (int)theight);
		crepe_draw_context_border(dc, dc->ui, inner[0], inner[1], (size_t)inner[2], (size_t)inner[3], widget->width - TITLE_BORDER_WIDTH*2, widget->height - tstart - TITLE_BORDER_WIDTH - theight, 1);
	}
	if (window->title) crepe_widget_draw(window->title, dc);
	if (window->widget) crepe_widget_draw(window->widget, dc);

	size_t ncommands = dc->ncommands;
	crepe_draw_context_end(dc);
	
	if (ncommands) wm_post_window(window->window);
}

/* destroy window */
static void window_destroy(crepe_widget_t *widget) {

	crepe_window_t *window = CREPE_WINDOW(widget);

	if (window == window->context->first) window->context->first = window->next;
	if (window == window->context->last) window->context->last = window->previous;
	if (window->previous) window->previous->next = window->next;
	if (window->next) window->next->previous = window->previous;

	crepe_widget_free(window->widget);
	crepe_widget_free(window->title);

	if (window->image) wm_destroy_image(window->image);
	wm_destroy_window(window->window);
}

/* close pressed callback */

/* create window */
extern crepe_widget_t *crepe_window_new(crepe_context_t *context, size_t mwidth, size_t mheight) {

	uint32_t wid = wm_create_window();
	if (!wid) return NULL;

	/* create window structure */
	crepe_window_t *window = CREPE_WINDOW(crepe_widget_new(&crepe_widget_ops_window));

	window->context = context;
	window->previous = context->last;
	if (!context->first) context->first = window;
	if (context->last) context->last->next = window;
	context->last = window;
	window->next = NULL;
	window->title = NULL;
	window->widget = NULL;
	window->window = wid;
	window->image = WM_NULL;
	window->visible = false;
	window->mwidth = mwidth;
	window->mheight = mheight;
	window->close = false;

	return CREPE_WIDGET(window);
}

/* set title bar widget */
extern void crepe_window_title(crepe_window_t *window, crepe_widget_t *title) {

	window->title = title;
}

/* set central widget */
extern void crepe_window_child(crepe_window_t *window, crepe_widget_t *widget) {

	window->widget = widget;
}

/* present window contents */
extern void crepe_window_present(crepe_window_t *window) {

	crepe_widget_t *widget = CREPE_WIDGET(window);

	crepe_widget_minimum_size(widget, window->context->dc);
	crepe_widget_final_size(widget, window->context->dc, 0, 0);
	crepe_widget_position(widget, NULL);

	/* create image */
	if (!window->image) {

		window->image = wm_create_image((uint32_t)window->base.width, (uint32_t)window->base.height, WM_CLASS_WINDOW);
		if (!window->image) return;

		wm_window_attributes_t attributes = {
			.x = ((uint32_t)widget->halign * (window->context->scinfo.width - widget->width)) / 2,
			.y = ((uint32_t)widget->valign * (window->context->scinfo.height - widget->height)) / 2,
			.width = (uint32_t)widget->width,
			.height = (uint32_t)widget->height,
			.events = 0,
			.image = window->image,
			.stack = WM_STACK_CENTER,
		};
		wm_set_window_attributes(window->window, WM_WINDOW_ATTRIBUTE_ALL, &attributes);
	}

	/* draw image */
	crepe_widget_draw(CREPE_WIDGET(window), window->context->dc);
}

/* set window to close */
extern void crepe_window_close(crepe_window_t *window) {

	window->close = true;
}

/* initialize context */
extern crepe_result_t crepe_context_init(crepe_context_t *context) {

	if (!context || context->init) return CREPE_RESULT_FAILURE;

	if (wm_open() < 0)
		return CREPE_RESULT_FAILURE;
	wm_get_screen_info(&context->scinfo);
	context->init = true;

	context->first = NULL;
	context->last = NULL;
	context->dc = crepe_draw_context_new();
	if (!context->dc) return CREPE_RESULT_FAILURE;

	return CREPE_RESULT_SUCCESS;
}

/* main context loop */
extern void crepe_context_main_loop(crepe_context_t *context) {

	while (context->first) {

		crepe_window_t *window = context->first;
		while (window) {

			crepe_window_t *next = window->next;

			uint32_t count = 0;
			wm_event_t *events = wm_get_queued_window_events(window->window, &count);
			for (uint32_t i = 0; events && i < count; i++)
				crepe_widget_process_event(CREPE_WIDGET(window), events+i);

			crepe_widget_draw(CREPE_WIDGET(window), context->dc);
			if (window->close) crepe_widget_free(CREPE_WIDGET(window));

			window = next;
		}

		ec_timeval_t tv = {
			.sec = 0,
			.nsec = 1000000000 / FPS,
		};
		ec_sleepns(&tv);
	}
}

/* destroy context */
extern void crepe_context_destroy(crepe_context_t *context) {

	if (!context || !context->init)
		return;

	crepe_window_t *cur = context->first;
	while (cur) {

		crepe_window_t *next = cur->next;
		crepe_widget_free(CREPE_WIDGET(cur));
		cur = next;
	}
	crepe_draw_context_free(context->dc);
	wm_close();
	context->init = false;
}
