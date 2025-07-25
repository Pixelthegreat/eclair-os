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
#include <ec/keycode.h>
#include <ec/device.h>
#include <ec/wm.h>
#include <wm/input.h>

#define KBD_PATH "/dev/kbd0"
#define MOUSE_PATH "/dev/mus0"

static int kfd = -1; /* keyboard fd */
static int mfd = -1; /* mouse fd */

#define MAX_EVENTS 64
static wm_event_t events[MAX_EVENTS]; /* event ringbuffer */
static size_t ev_start = 0, ev_end = 0; /* ringbuffer positions */

static bool state[ECB_COUNT]; /* button state */

/* initialize input */
extern int input_init(void) {

	kfd = ec_open(KBD_PATH, ECF_READ, 0);
	if (kfd < 0) fprintf(stderr, "Can't open '" KBD_PATH "': %s\n", strerror(errno));
	else ec_ioctl(kfd, ECIO_INP_FLUSH, 0);

	mfd = ec_open(MOUSE_PATH, ECF_READ, 0);
	if (mfd < 0) fprintf(stderr, "Can't open '" MOUSE_PATH "': %s\n", strerror(errno));
	else ec_ioctl(mfd, ECIO_INP_FLUSH, 0);

	if (kfd < 0 && mfd < 0) return -1;
	return 0;
}

/* update input */
extern void input_update(void) {

	/* get key events */
	int keycode = ec_ioctl(kfd, ECIO_INP_GETEVENT, 0);
	while (keycode > 0 && kfd >= 0) {

		wm_event_t *event = input_get_last_event();

		event->type = WM_EVENT_KEY;
		event->key.code = (uint32_t)(keycode & ECK_CODE);
		event->key.action = (keycode & ECK_RELEASE)? WM_ACTION_RELEASED: WM_ACTION_PRESSED;

		keycode = ec_ioctl(kfd, ECIO_INP_GETEVENT, 0);
	}

	/* get mouse events */
	ec_msevent_t msevent;
	while (!ec_ioctl(mfd, ECIO_INP_GETEVENT, (uintptr_t)&msevent)) {

		/* motion */
		if (msevent.x || msevent.y) {

			wm_event_t *event = input_get_last_event();

			event->type = WM_EVENT_MOTION;
			event->motion.move.x = (int32_t)msevent.x;
			event->motion.move.y = (int32_t)msevent.y;
			event->motion.position.x = 0;
			event->motion.position.y = 0;
		}

		/* buttons */
		for (uint32_t i = 0; i < ECB_COUNT; i++) {
			if (state[i] != msevent.state[i]) {

				wm_event_t *event = input_get_last_event();

				event->type = WM_EVENT_BUTTON;
				event->button.code = i;
				event->button.action = msevent.state[i]? WM_ACTION_PRESSED: WM_ACTION_RELEASED;

				state[i] = msevent.state[i];
			}
		}
	}
}

/* get next input event */
extern wm_event_t *input_get_next_event(void) {

	if (ev_start == ev_end)
		return NULL;

	wm_event_t *event = &events[ev_start];

	ev_start = (ev_start + 1) % MAX_EVENTS;
	return event;
}

/* get last input event */
extern wm_event_t *input_get_last_event(void) {

	wm_event_t *event = &events[ev_end];

	ev_end = (ev_end + 1) % MAX_EVENTS;
	return event;
}
