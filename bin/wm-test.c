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

	if (wm_open() < 0) {

		fprintf(stderr, "Can't connect to window manager: %s\n", strerror(errno));
		return 1;
	}

	/* create window */
	window = wm_create_window();
	if (!window) return 1;

	/* create image */
	image = wm_create_image(WIDTH, HEIGHT, WM_CLASS_WINDOW);
	if (!image) return 1;

	/* fill image with color */
	wm_draw_command_t command = {
		.type = WM_DRAW_FILL,
		.fill.color = {0xff, 0xff, 0xff},
	};
	if (wm_draw_batch(image, 1, &command) < 0)
		return 1;

	/* set window attributes */
	wm_window_attributes_t attributes = {
		.x = 24, .y = 24,
		.width = WIDTH, .height = HEIGHT,
		.events = 0,
		.image = image,
		.stack = WM_STACK_CENTER,
	};
	if (wm_set_window_attributes(window, WM_WINDOW_ATTRIBUTE_ALL, &attributes) < 0)
		return 1;

	/* main loop */
	bool running = true;
	while (running) {

		uint32_t count = 0;
		wm_event_t *events = wm_get_queued_window_events(window, &count);
		if (!events) return 1;

		/* interpret events */
		for (uint32_t i = 0; i < count; i++) {

			wm_event_t *event = events+i;
			switch (event->type) {

				/* key press/release */
				case WM_EVENT_KEY:
					if (event->key.action == WM_ACTION_PRESSED && event->key.code == ECK_ESCAPE)
						running = false;
					break;
				/* button press/release */
				case WM_EVENT_BUTTON:
					if (event->button.action == WM_ACTION_PRESSED && event->button.code == ECB_RIGHT)
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

	if (window) wm_destroy_window(window);
	if (image) wm_destroy_image(image);
	if (fd >= 0) wm_close();
}

int main() {

	int code = run();
	cleanup();
	return code;
}
