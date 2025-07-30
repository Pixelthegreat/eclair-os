/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec.h>
#include <ec/device.h>
#include <ec/wm.h>
#include <wm/input.h>
#include <wm/screen.h>
#include <wm/window.h>
#include <wm/server.h>

static int fd = -1; /* channel fd */
static int pid; /* message source */
static char buffer[ECIO_CHNL_BUFSZ]; /* message buffer */
static char resbuffer[ECIO_CHNL_BUFSZ]; /* response buffer if needed */
static wm_message_t *message = (wm_message_t *)buffer; /* message header */
static size_t size = 0; /* message size */

#define MAX_RESOURCES 1024
static resource_t resources[MAX_RESOURCES]; /* resource pool */

#define TPS 60

static wm_message_t result = {
	.type = WM_RESPONSE,
	.size = sizeof(result),
	.result = WM_SUCCESS,
}; /* generic result message */

/* send response message */
static void send_response(wm_message_t *response, size_t rsize) {

	ec_ioctl(fd, ECIO_CHNL_SETDEST, (uintptr_t)pid);
	ec_write(fd, response, rsize);
}

/* message function handlers */
static void handle_acknowledge(void);
static void handle_forget(void);
static void handle_create_image(void);
static void handle_destroy_image(void);
static void handle_resize_image(void);
static void handle_set_image_data(void);
static void handle_create_window(void);
static void handle_destroy_window(void);
static void handle_set_window_attributes(void);
static void handle_get_window_attributes(void);
static void handle_get_queued_window_events(void);
static void handle_post_window(void);

static void (*handlers[])(void) = {
	[WM_FUNCTION_ACKNOWLEDGE] = handle_acknowledge,
	[WM_FUNCTION_FORGET] = handle_forget,
	[WM_FUNCTION_CREATE_IMAGE] = handle_create_image,
	[WM_FUNCTION_DESTROY_IMAGE] = handle_destroy_image,
	[WM_FUNCTION_RESIZE_IMAGE] = handle_resize_image,
	[WM_FUNCTION_SET_IMAGE_DATA] = handle_set_image_data,
	[WM_FUNCTION_CREATE_WINDOW] = handle_create_window,
	[WM_FUNCTION_DESTROY_WINDOW] = handle_destroy_window,
	[WM_FUNCTION_RESIZE_IMAGE] = handle_resize_image,
	[WM_FUNCTION_SET_IMAGE_DATA] = handle_set_image_data,
	[WM_FUNCTION_CREATE_WINDOW] = handle_create_window,
	[WM_FUNCTION_DESTROY_WINDOW] = handle_destroy_window,
	[WM_FUNCTION_SET_WINDOW_ATTRIBUTES] = handle_set_window_attributes,
	[WM_FUNCTION_GET_WINDOW_ATTRIBUTES] = handle_get_window_attributes,
	[WM_FUNCTION_GET_QUEUED_WINDOW_EVENTS] = handle_get_queued_window_events,
	[WM_FUNCTION_POST_WINDOW] = handle_post_window,
};

/* acknowledge connection */
static void handle_acknowledge(void) {

	result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
}

/* forget connection */
static void handle_forget(void) {

	result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
}

/* create image */
static void handle_create_image(void) {

	wm_create_image_request_t *request = (wm_create_image_request_t *)message;
	resource_t *resource = image_resource_new(request->cls, (size_t)request->width, (size_t)request->height);
	if (!resource) {

		result.result = WM_NULL;
		send_response(&result, sizeof(result));
		return;
	}
	resource->owner = pid;
	result.result = resource->id;
	send_response(&result, sizeof(result));
}

/* destroy image */
static void handle_destroy_image(void) {

	wm_destroy_image_request_t *request = (wm_destroy_image_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_IMAGE || resource->refcnt) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}
	image_resource_free(resource);
	result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
}

/* resize image */
static void handle_resize_image(void) {

	wm_resize_image_request_t *request = (wm_resize_image_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_IMAGE) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}
	image_resource_resize(resource, (size_t)request->width, (size_t)request->height);
	result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
}

/* set image data */
static void handle_set_image_data(void) {

	wm_set_image_data_request_t *request = (wm_set_image_data_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_IMAGE ||
	    request->format < WM_FORMAT_RGB8 || request->format > WM_FORMAT_RGBA8 ||
	    request->size != ((uint32_t)size - sizeof(wm_set_image_data_request_t)) / (request->format + 2)) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}
	image_t *image = image_resource_get_image(resource);
	image_set_data(image, request->format, (size_t)request->offset, (size_t)request->size, request->data);
	result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
}

/* create window */
static void handle_create_window(void) {

	resource_t *resource = window_resource_new();
	if (!resource) {

		result.result = WM_NULL;
		send_response(&result, sizeof(result));
		return;
	}
	resource->owner = pid;
	result.result = resource->id;
	send_response(&result, sizeof(result));
}

/* destroy window */
static void handle_destroy_window(void) {

	wm_destroy_window_request_t *request = (wm_destroy_window_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_WINDOW) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}
	window_resource_free(resource);
	result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
}

/* set window attributes */
static void handle_set_window_attributes(void) {

	wm_set_window_attributes_request_t *request = (wm_set_window_attributes_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_WINDOW ||
	    window_resource_set_attributes(resource, &request->attributes, request->mask) < 0)
		result.result = WM_FAILURE;
	else result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
}

/* get window attributes */
static void handle_get_window_attributes(void) {

	wm_get_window_attributes_request_t *request = (wm_get_window_attributes_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_WINDOW) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}

	wm_get_window_attributes_response_t response = {
		.base = {
			.type = WM_RESPONSE,
			.size = sizeof(response),
			.result = WM_SUCCESS,
		},
	};
	memset(&response.attributes, 0, sizeof(response.attributes));
	window_resource_get_attributes(resource, &response.attributes, request->mask);
	send_response(&response.base, sizeof(response));
}

/* get queued window events */
static void handle_get_queued_window_events(void) {

	wm_get_queued_window_events_request_t *request = (wm_get_queued_window_events_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_WINDOW) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}

	wm_get_queued_window_events_response_t *response = (wm_get_queued_window_events_response_t *)resbuffer;
	response->base.type = WM_RESPONSE;
	response->base.result = WM_SUCCESS;
	response->returned = 0;
	response->remaining = 0;

	wm_event_t *event;
	size_t size = sizeof(wm_get_queued_window_events_response_t);
	while (size <= ECIO_CHNL_BUFSZ-sizeof(wm_event_t) &&
	       (event = window_resource_get_next_event(resource)) != NULL) {

		memcpy(&response->events[response->returned], event, sizeof(wm_event_t));
		response->returned++;
		size += sizeof(wm_event_t);
	}
	response->base.size = (uint32_t)size;
	send_response(&response->base, size);
}

/* post window */
static void handle_post_window(void) {

	wm_post_window_request_t *request = (wm_post_window_request_t *)message;
	resource_t *resource = server_get_resource(request->id);
	if (!resource || resource->type != WM_RESOURCE_WINDOW) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}

	window_draw = true;
	result.result = WM_SUCCESS;
	send_response(&result, sizeof(result));
	return;
}

/* process generic message */
static void process_message(void) {

	if ((size_t)message->size != size || message->type != WM_REQUEST ||
	    message->function >= (sizeof(handlers) / sizeof(handlers[0])) ||
	    !handlers[message->function]) {

		result.result = WM_FAILURE;
		send_response(&result, sizeof(result));
		return;
	}
	handlers[message->function]();
}

/* process event */
static void process_event(wm_event_t *event) {

	if (!screen_process_event(event))
		return;
	if (!window_process_event(event))
		return;
}

/* host server */
extern int server_host(void) {

	char buf[32];
	snprintf(buf, 32, "/dev/chnl/%d", WM_CHNL);

	fd = ec_open(buf, ECF_READ | ECF_WRITE, 0);
	if (fd < 0) {

		fprintf(stderr, "Can't open '%s': %s\n", buf, strerror(errno));
		return -1;
	}
	return 0;
}

/* allocate resource */
extern resource_t *server_create_resource(uint32_t type) {

	for (uint32_t i = 1; i < MAX_RESOURCES; i++) {
		if (!resources[i].id) {

			resources[i].id = i;
			resources[i].type = type;
			resources[i].refcnt = 0;
			resources[i].owner = 0;
			resources[i].data = 0;

			return &resources[i];
		}
	}
	return NULL;
}

/* get resource by id */
extern resource_t *server_get_resource(uint32_t id) {

	if (id < MAX_RESOURCES && resources[id].id)
		return &resources[id];
	return NULL;
}

/* deallocate resource */
extern void server_destroy_resource(resource_t *resource) {

	resource->id = 0;
}

/* update server */
extern int server_update(void) {

	ec_timeval_t tv = {
		.sec = 0,
		.nsec = 1000000000 / TPS,
	};
	if (ec_ioctl(fd, ECIO_CHNL_WAITREAD, (uintptr_t)&tv) >= 0) {

		pid = ec_ioctl(fd, ECIO_CHNL_GETSOURCE, 0);
		ec_ioctl(fd, ECIO_CHNL_LOCKW, 0);

		size = (size_t)ec_read(fd, buffer, ECIO_CHNL_BUFSZ);
		process_message();

		ec_ioctl(fd, ECIO_CHNL_UNLOCKW, 0);
	}
	input_update();

	wm_event_t *event = NULL;
	while ((event = input_get_next_event()) != NULL)
		process_event(event);

	if (window_draw) {

		screen_begin();
		window_update();
		screen_end();
	}
	return 0;
}
