/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wm/server.h>
#include <wm/window.h>

bool window_draw = false; /* need to draw windows */

#define MAX_WINDOWS 1024
static uint32_t windows[MAX_WINDOWS]; /* window stack */
static int nwindows; /* number of windows */
static uint32_t focus; /* focused window */
static uint32_t oldfocus; /* previously focused window */
static int firstcenter; /* index of first center of stack window */
static int firstabove; /* index of first "always above" window */

/* create image */
extern resource_t *image_resource_new(uint32_t cls, size_t width, size_t height) {

	resource_t *resource = server_create_resource(0);
	if (!resource) return NULL;

	image_resource_t *image = (image_resource_t *)malloc(sizeof(image_resource_t));

	image->cls = cls;
	image->image = IMAGE_INIT;
	image_init(&image->image, width, height);

	resource->data = image;
	resource->type = WM_RESOURCE_IMAGE;
	return resource;
}

/* resize image */
extern void image_resource_resize(resource_t *resource, size_t width, size_t height) {

	image_resource_t *image = (image_resource_t *)resource->data;
	image_resize(&image->image, width, height);
}

/* get image from resource */
extern image_t *image_resource_get_image(resource_t *resource) {

	return &((image_resource_t *)resource->data)->image;
}

/* free image */
extern void image_resource_free(resource_t *resource) {

	image_resource_t *image = (image_resource_t *)resource->data;
	image_destroy(&image->image);

	free(image);
	server_destroy_resource(resource);
}

/* create window */
extern resource_t *window_resource_new(void) {

	if (nwindows >= MAX_WINDOWS) return NULL;

	resource_t *resource = server_create_resource(0);
	if (!resource) return NULL;

	window_resource_t *window = (window_resource_t *)malloc(sizeof(window_resource_t));

	window->attributes.x = 0;
	window->attributes.y = 0;
	window->attributes.width = 0;
	window->attributes.height = 0;
	window->attributes.events = 0;
	window->attributes.image = WM_NULL;
	window->attributes.stack = WM_STACK_CENTER;
	window->ox = 0;
	window->oy = 0;
	window->image = NULL;
	window->ev_start = 0;
	window->ev_end = 0;
	window->visible = true;

	/* add window to stack */
	if (nwindows) {
		for (int i = nwindows-1; i >= firstabove; i--)
			windows[i+1] = windows[i];
	}
	windows[firstabove++] = resource->id;
	nwindows++;
	oldfocus = focus;
	focus = resource->id;
	window_draw = true;

	resource->data = window;
	resource->type = WM_RESOURCE_WINDOW;
	return resource;
}

/* set window attributes */
extern int window_resource_set_attributes(resource_t *resource, wm_window_attributes_t *attributes, uint32_t mask) {

	window_resource_t *window = (window_resource_t *)resource->data;

	/* position */
	if (mask & WM_WINDOW_ATTRIBUTE_POSITION) {

		if (resource->id != focus)
			return -1;

		window->ox = window->attributes.x;
		window->oy = window->attributes.y;
		window->attributes.x = attributes->x;
		window->attributes.y = attributes->y;

		window_draw = true;
	}

	/* size */
	if (mask & WM_WINDOW_ATTRIBUTE_SIZE) {

		if (resource->id != focus)
			return -1;

		window->attributes.width = attributes->width;
		window->attributes.height = attributes->height;

		window_draw = true;
	}

	/* event mask */
	if (mask & WM_WINDOW_ATTRIBUTE_EVENTS)
		window->attributes.events = attributes->events;

	/* image */
	if (mask & WM_WINDOW_ATTRIBUTE_IMAGE) {

		resource_t *image = server_get_resource(attributes->image);
		if (!image || ((image_resource_t *)image->data)->cls != WM_CLASS_WINDOW)
			return -1;

		if (window->image) window->image->refcnt--;

		window->attributes.image = attributes->image;
		window->image = image;
		window->image->refcnt++;

		if (resource->id == focus) window_draw = true;
	}

	/* stack position */
	if (mask & WM_WINDOW_ATTRIBUTE_STACK) {

		if (window->attributes.stack != WM_STACK_CENTER)
			return -1;

		window->attributes.stack = attributes->stack;

		/* move to below section */
		if (attributes->stack == WM_STACK_BELOW) {

			for (int i = firstabove-1; i > firstcenter; i++)
				windows[i] = windows[i-1];
			windows[firstcenter++] = resource->id;
		}

		/* move to above section */
		else if (attributes->stack == WM_STACK_ABOVE) {

			for (int i = firstabove-1; i < nwindows-1; i++)
				windows[i] = windows[i+1];
			windows[nwindows-1] = resource->id;
			firstabove--;
		}

		/* refocus */
		if (resource->id == focus && attributes->stack != WM_STACK_CENTER) {

			oldfocus = focus;
			focus = firstcenter < firstabove? windows[firstcenter]: WM_NULL;
		}
		window_draw = true;
	}

	return 0;
}

/* get window attributes */
extern void window_resource_get_attributes(resource_t *resource, wm_window_attributes_t *attributes, uint32_t mask) {

	window_resource_t *window = (window_resource_t *)resource->data;

	/* position */
	if (mask & WM_WINDOW_ATTRIBUTE_POSITION) {

		attributes->x = window->attributes.x;
		attributes->y = window->attributes.y;
	}

	/* size */
	if (mask & WM_WINDOW_ATTRIBUTE_SIZE) {

		attributes->width = window->attributes.width;
		attributes->height = window->attributes.height;
	}

	/* event mask */
	if (mask & WM_WINDOW_ATTRIBUTE_EVENTS)
		attributes->events = window->attributes.events;

	/* image */
	if (mask & WM_WINDOW_ATTRIBUTE_IMAGE)
		attributes->image = window->attributes.image;

	/* stack position */
	if (mask & WM_WINDOW_ATTRIBUTE_STACK)
		attributes->stack = window->attributes.stack;
}

/* get next window event */
extern wm_event_t *window_resource_get_next_event(resource_t *resource) {

	window_resource_t *window = (window_resource_t *)resource->data;

	if (window->ev_start == window->ev_end)
		return NULL;
	wm_event_t *event = &window->events[window->ev_start];

	window->ev_start = (window->ev_start + 1) % WINDOW_RESOURCE_MAX_EVENTS;
	return event;
}

/* get last window event */
extern wm_event_t *window_resource_get_last_event(resource_t *resource) {

	window_resource_t *window = (window_resource_t *)resource->data;
	wm_event_t *event = &window->events[window->ev_end];

	window->ev_end = (window->ev_end + 1) % WINDOW_RESOURCE_MAX_EVENTS;
	return event;
}

/* free window */
extern void window_resource_free(resource_t *resource) {

	int i = 0;
	for (; i < nwindows && windows[i] != resource->id; i++);
	if (i < nwindows) {

		for (int j = i; j < nwindows-1; j++)
			windows[j] = windows[j+1];
		if (i < firstabove) firstabove--;
		if (i < firstcenter) firstcenter--;
		nwindows--;
	}

	/* refocus */
	if (resource->id == focus) {

		oldfocus = focus;
		focus = firstcenter < firstabove? windows[firstabove-1]: WM_NULL;
		window_draw = true;
	}

	/* free resources */
	window_resource_t *window = (window_resource_t *)resource->data;

	if (window->image) window->image->refcnt++;
	free(window);

	server_destroy_resource(resource);
}

/* update windows */
extern void window_update(void) {

	resource_t *rfocus = server_get_resource(focus);
	window_resource_t *wfocus = rfocus? (window_resource_t *)rfocus->data: NULL;

	/* redraw all */
	if (focus != oldfocus) {

		color_t color = {0, 0, 0};
		screen_clear(color);

		for (int i = 0; i < nwindows; i++) {

			window_resource_t *window = (window_resource_t *)server_get_resource(windows[i])->data;
			if (!window->image) return;

			image_resource_t *image = (image_resource_t *)window->image->data;
			image_copy_area(&screen_image, &image->image, window->attributes.x, window->attributes.y, 0, 0, (int)image->image.width, (int)image->image.height);

			window->visible = true;
		}
	}

	/* redraw focused */
	else if (wfocus && wfocus->image) {

		image_resource_t *image = (image_resource_t *)wfocus->image->data;
		image_copy_area(&screen_image, &image->image, wfocus->attributes.x, wfocus->attributes.y, 0, 0, (int)image->image.width, (int)image->image.height);

		wfocus->visible = true;
	}
	oldfocus = focus;
	window_draw = false;
}

/* process event */
extern bool window_process_event(wm_event_t *event) {

	int x = 0, y = 0;
	uint32_t wid = focus;

	switch (event->type) {
		/* mouse button */
		case WM_EVENT_BUTTON:
			if (!nwindows || event->button.action != WM_ACTION_PRESSED)
				break;
			for (int i = nwindows-1; i >= 0; i--) {

				window_resource_t *window = (window_resource_t *)server_get_resource(windows[i])->data;
				int rx = event->button.position.x - window->attributes.x;
				int ry = event->button.position.y - window->attributes.y;

				if (rx < 0 || rx >= window->attributes.width ||
				    ry < 0 || ry >= window->attributes.height)
					continue;

				x = rx;
				y = ry;
				wid = windows[i];

				/* refocus */
				if (i >= firstcenter && i < firstabove && windows[i] != focus) {

					oldfocus = focus;
					focus = windows[i];
					for (int j = i; j < firstabove-1; j++)
						windows[j] = windows[j+1];
					windows[firstabove-1] = focus;

					window_draw = true;
				}
				break;
			}
			break;
		/* mouse motion */
		case WM_EVENT_MOTION:
			if (!nwindows) break;
			for (int i = nwindows-1; i >= 0; i--) {

				window_resource_t *window = (window_resource_t *)server_get_resource(windows[i])->data;
				int rx = event->motion.position.x - window->attributes.x;
				int ry = event->motion.position.y - window->attributes.y;

				if (rx < 0 || rx >= window->attributes.width ||
				    ry < 0 || ry >= window->attributes.height)
					continue;

				x = rx;
				y = rx;
				wid = windows[i];
				break;
			}
			break;
	}

	/* add event */
	resource_t *resource = server_get_resource(wid);
	if (!resource) return true;

	wm_event_t *new = window_resource_get_last_event(resource);
	memcpy(new, event, sizeof(wm_event_t));

	/* set position */
	switch (event->type) {
		case WM_EVENT_BUTTON:
			new->button.position.x = x;
			new->button.position.y = y;
			break;
		case WM_EVENT_MOTION:
			new->motion.position.x = x;
			new->motion.position.y = y;
			break;
	}
	return false;
}
