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
static wm_message_t *message = (wm_message_t *)buffer; /* message header */
static size_t size = 0; /* message size */

#define MAX_RESOURCES 1024
static resource_t resources[MAX_RESOURCES]; /* resource pool */

#define TPS 60

/* send response message */
static void send_response(wm_message_t *response, size_t rsize) {

	ec_ioctl(fd, ECIO_CHNL_SETDEST, (uintptr_t)pid);
	ec_write(fd, response, size);
}

/* process generic message */
static void process_message(void) {

	if ((size_t)message->size != size || message->type != WM_REQUEST)
		return;

	printf("[host] Request: Type=%u, Size=%u, Function=%u\n", message->type, message->size, message->function);

	wm_message_t response = {
		.type = WM_RESPONSE,
		.size = sizeof(response),
		.result = WM_SUCCESS,
	};
	send_response(&response, sizeof(response));
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
