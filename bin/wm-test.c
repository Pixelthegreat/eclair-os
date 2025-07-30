#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ec.h>
#include <ec/device.h>
#include <ec/keycode.h>
#include <ec/wm.h>

static int fd = -1; /* channel fd */
static char buffer[ECIO_CHNL_BUFSZ]; /* message response buffer */
static char reqbuffer[ECIO_CHNL_BUFSZ]; /* request buffer if needed */
static wm_message_t *response = (wm_message_t *)buffer; /* message response */
static size_t rsize; /* response size */
static uint32_t window; /* window handle */
static uint32_t image; /* window image */

#define WIDTH 128
#define HEIGHT 128

static int send_message(wm_message_t *message, size_t size) {

	while (ec_write(fd, message, size) < 0 && errno == EAGAIN);

	ec_ioctl(fd, ECIO_CHNL_WAITREAD, 0);
	rsize = (size_t)ec_read(fd, buffer, ECIO_CHNL_BUFSZ);

	if (!response->result)
		return -1;
	return 0;
}

/* run application */
static int run(void) {

	char pathbuf[32];
	snprintf(pathbuf, 32, "/dev/chnl/%d", WM_CHNL);

	fd = ec_open(pathbuf, ECF_READ | ECF_WRITE, 0);
	if (fd < 0) {

		fprintf(stderr, "Can't open '%s': %s\n", pathbuf, strerror(errno));
		return 1;
	}

	/* acknowledge connection */
	wm_message_t acknowledge = {
		.type = WM_REQUEST,
		.size = sizeof(acknowledge),
		.function = WM_FUNCTION_ACKNOWLEDGE,
	};
	if (send_message(&acknowledge, sizeof(acknowledge)) < 0)
		return 1;

	/* create window */
	wm_message_t create_window = {
		.type = WM_REQUEST,
		.size = sizeof(create_window),
		.function = WM_FUNCTION_CREATE_WINDOW,
	};
	if (send_message(&create_window, sizeof(create_window)) < 0)
		return 1;
	window = response->result;

	/* create image */
	wm_create_image_request_t create_image = {
		.base = {
			.type = WM_REQUEST,
			.size = sizeof(create_image),
			.function = WM_FUNCTION_CREATE_IMAGE,
		},
		.width = WIDTH,
		.height = HEIGHT,
		.cls = WM_CLASS_WINDOW,
	};
	if (send_message(&create_image.base, sizeof(create_image)) < 0)
		return 1;
	image = response->result;

	/* set image data */
	size_t npixels = 0;
	size_t ntotal = WIDTH * HEIGHT;
	size_t max = ECIO_CHNL_BUFSZ - sizeof(wm_set_image_data_request_t);
	size_t nmax = max / 3;

	wm_set_image_data_request_t *set_image_data = (wm_set_image_data_request_t *)reqbuffer;

	set_image_data->base.type = WM_REQUEST;
	set_image_data->base.size = sizeof(wm_set_image_data_request_t);
	set_image_data->base.function = WM_FUNCTION_SET_IMAGE_DATA;
	set_image_data->id = image;
	set_image_data->format = WM_FORMAT_RGB8;
	set_image_data->offset = 0;
	set_image_data->size = 0;

	while (npixels < ntotal) {

		size_t amount = ntotal - npixels > nmax? nmax: ntotal - npixels;

		for (size_t i = 0; i < amount; i++) {

			set_image_data->data[i * 3] = 0xff;
			set_image_data->data[i * 3 + 1] = 0xff;
			set_image_data->data[i * 3 + 2] = 0xff;
		}
		set_image_data->offset = (uint32_t)npixels;
		set_image_data->size = (uint32_t)amount;
		set_image_data->base.size = sizeof(wm_set_image_data_request_t) + (uint32_t)amount * 3;

		npixels += amount;
		if (send_message(&set_image_data->base, (size_t)set_image_data->base.size) < 0)
			return 1;
	}

	/* set window attributes */
	wm_set_window_attributes_request_t set_window_attributes = {
		.base = {
			.type = WM_REQUEST,
			.size = sizeof(set_window_attributes),
			.function = WM_FUNCTION_SET_WINDOW_ATTRIBUTES,
		},
		.id = window,
		.mask = WM_WINDOW_ATTRIBUTE_ALL,
		.attributes = {
			.x = 24, .y = 24,
			.width = WIDTH, .height = HEIGHT,
			.events = 0,
			.image = image,
			.stack = WM_STACK_CENTER,
		},
	};
	if (send_message(&set_window_attributes.base, sizeof(set_window_attributes)) < 0)
		return 1;

	/* main loop */
	bool running = true;
	while (running) {

		wm_get_queued_window_events_request_t get_queued_window_events = {
			.base = {
				.type = WM_REQUEST,
				.size = sizeof(get_queued_window_events),
				.function = WM_FUNCTION_GET_QUEUED_WINDOW_EVENTS,
			},
			.id = window,
		};
		if (send_message(&get_queued_window_events.base, sizeof(get_queued_window_events)) < 0)
			return 1;

		/* interpret events */
		wm_get_queued_window_events_response_t *events = (wm_get_queued_window_events_response_t *)response;
		for (uint32_t i = 0; i < events->returned; i++) {

			wm_event_t *event = &events->events[i];
			switch (event->type) {

				/* key press/release */
				case WM_EVENT_KEY:
					if (event->key.action == WM_ACTION_PRESSED && event->key.code == ECK_ESCAPE)
						running = false;
					break;
			}
		}

		/* fps delay */
		ec_timeval_t tv = {
			.sec = 0,
			.nsec = 1000000000 / 20,
		};
		ec_sleepns(&tv);
	}
	return 0;
}

/* clean up resources */
static void cleanup(void) {

	if (fd >= 0) {

		/* destroy window */
		if (window) {

			wm_destroy_window_request_t request = {
				.base = {
					.type = WM_REQUEST,
					.size = sizeof(request),
					.function = WM_FUNCTION_DESTROY_WINDOW,
				},
				.id = window,
			};
			send_message(&request.base, sizeof(request));
		}

		/* destroy image */
		if (image) {

			wm_destroy_image_request_t request = {
				.base = {
					.type = WM_REQUEST,
					.size = sizeof(request),
					.function = WM_FUNCTION_DESTROY_IMAGE,
				},
				.id = image,
			};
			send_message(&request.base, sizeof(request));
		}

		/* forget connection */
		wm_message_t forget = {
			.type = WM_REQUEST,
			.size = sizeof(forget),
			.function = WM_FUNCTION_FORGET,
		};
		send_message(&forget, sizeof(forget));
		ec_close(fd);
	}
}

int main() {

	int code = run();
	cleanup();
	return code;
}
