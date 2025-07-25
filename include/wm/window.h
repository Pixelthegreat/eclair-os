/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WM_WINDOW_H
#define WM_WINDOW_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <ec/wm.h>
#include <wm/screen.h>
#include <wm/server.h>

/* image resource */
typedef struct image_resource {
	uint32_t cls; /* image class */
	image_t image; /* image */
} image_resource_t;

/* window resource */
#define WINDOW_RESOURCE_MAX_EVENTS 32

typedef struct window_resource {
	wm_window_attributes_t attributes; /* window attributes */
	int ox, oy; /* old position */
	resource_t *image; /* associated image */
	wm_event_t events[WINDOW_RESOURCE_MAX_EVENTS]; /* event ringbuffer */
	size_t ev_start, ev_end; /* ringbuffer positions */
	bool visible; /* visible */
} window_resource_t;

extern bool window_draw; /* need to draw windows */

/* functions */
extern resource_t *image_resource_new(uint32_t cls, size_t width, size_t height); /* create image */
extern void image_resource_resize(resource_t *resource, size_t width, size_t height); /* resize image */
extern image_t *image_resource_get_image(resource_t *resource); /* get image from resource */
extern void image_resource_free(resource_t *resource); /* free image */

extern resource_t *window_resource_new(void); /* create window */
extern int window_resource_set_attributes(resource_t *resource, wm_window_attributes_t *attributes, uint32_t mask); /* set window attributes */
extern void window_resource_get_attributes(resource_t *resource, wm_window_attributes_t *attributes, uint32_t mask); /* get window attributes */
extern wm_event_t *window_resource_get_next_event(resource_t *resource); /* get next window event */
extern wm_event_t *window_resource_get_last_event(resource_t *resource); /* get last window event */
extern void window_resource_free(resource_t *resource); /* free window */

extern bool window_process_event(wm_event_t *event); /* process event */
extern void window_update(void); /* update windows */

#endif /* WM_WINDOW_H */
