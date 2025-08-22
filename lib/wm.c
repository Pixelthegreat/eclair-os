#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec.h>
#include <ec/device.h>
#include <ec/wm.h>

#define SETERRNO(e, r) ({\
		errno = e;\
		return r;\
	})

static int fd = -1; /* connection fd */
static char reqbuf[ECIO_CHNL_BUFSZ]; /* request buffer */
static char rspbuf[ECIO_CHNL_BUFSZ]; /* response buffer */
static size_t rspsize; /* response size */

/* send message */
static int send_message(wm_message_t *message, size_t size) {

	while (ec_write(fd, message, size) < 0 && errno == EAGAIN);

	ec_ioctl(fd, ECIO_CHNL_WAITREAD, 0);
	rspsize = (size_t)ec_read(fd, rspbuf, ECIO_CHNL_BUFSZ);

	wm_message_t *response = (wm_message_t *)rspbuf;
	if (!response->result)
		SETERRNO(-EINVAL, -1);
	return 0;
}

/* open wm connection */
extern int wm_open(void) {

	if (fd >= 0) SETERRNO(-EISCONN, -1);

	char buf[32];
	snprintf(buf, 32, "/dev/chnl/%d", WM_CHNL);
	fd = ec_open(buf, ECF_READ | ECF_WRITE, 0);
	if (fd < 0) return -1;

	/* send acknowledge */
	wm_message_t *message = (wm_message_t *)reqbuf;

	message->type = WM_REQUEST;
	message->size = sizeof(wm_message_t);
	message->function = WM_FUNCTION_ACKNOWLEDGE;

	return send_message(message, sizeof(wm_message_t));
}

/* close wm connection */
extern void wm_close(void) {

	if (fd < 0) return;

	/* send forget */
	wm_message_t *message = (wm_message_t *)reqbuf;

	message->type = WM_REQUEST;
	message->size = sizeof(wm_message_t);
	message->function = WM_FUNCTION_FORGET;

	send_message(message, sizeof(wm_message_t));

	/* close connection */
	ec_close(fd);
	fd = -1;
}

/* get screen info */
extern int wm_get_screen_info(wm_screen_info_t *info) {

	if (fd < 0) SETERRNO(-ENOTCONN, -1);

	wm_message_t *message = (wm_message_t *)reqbuf;

	message->type = WM_REQUEST;
	message->size = sizeof(wm_message_t);
	message->function = WM_FUNCTION_GET_SCREEN_INFO;

	if (send_message(message, sizeof(wm_message_t)) < 0)
		return -1;

	wm_get_screen_info_response_t *response = (wm_get_screen_info_response_t *)rspbuf;
	memcpy(info, &response->info, sizeof(wm_screen_info_t));
	return 0;
}

/* create image */
extern uint32_t wm_create_image(uint32_t width, uint32_t height, uint32_t cls) {

	if (fd < 0) SETERRNO(-ENOTCONN, WM_NULL);

	wm_create_image_request_t *message = (wm_create_image_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_create_image_request_t);
	message->base.function = WM_FUNCTION_CREATE_IMAGE;
	message->width = width;
	message->height = height;
	message->cls = cls;

	if (send_message(&message->base, sizeof(wm_create_image_request_t)) < 0)
		return WM_NULL;

	wm_message_t *response = (wm_message_t *)rspbuf;
	return response->result;
}

/* destroy image */
extern void wm_destroy_image(uint32_t id) {

	if (fd < 0) return;

	wm_destroy_image_request_t *message = (wm_destroy_image_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_destroy_image_request_t);
	message->base.function = WM_FUNCTION_DESTROY_IMAGE;
	message->id = id;

	send_message(&message->base, sizeof(wm_destroy_image_request_t));
}

/* resize image */
extern int wm_resize_image(uint32_t id, uint32_t width, uint32_t height) {

	if (fd < 0) SETERRNO(-ENOTCONN, -1);

	wm_resize_image_request_t *message = (wm_resize_image_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_resize_image_request_t);
	message->base.function = WM_FUNCTION_RESIZE_IMAGE;
	message->id = id;
	message->width = width;
	message->height = height;

	return send_message(&message->base, sizeof(wm_resize_image_request_t));
}

/* set image data */
extern int wm_set_image_data(uint32_t id, uint32_t format, uint32_t offset, uint32_t size, uint8_t *data) {

	if (fd < 0) SETERRNO(-ENOTCONN, -1);

	wm_set_image_data_request_t *message = (wm_set_image_data_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_set_image_data_request_t) + size * (format + 2);
	message->base.function = WM_FUNCTION_SET_IMAGE_DATA;
	message->id = id;
	message->format = format;
	message->offset = offset;
	message->size = size;
	memcpy(message->data, data, (size_t)(size * (format + 2)));

	return send_message(&message->base, (size_t)message->base.size);
}

/* create window */
extern uint32_t wm_create_window(void) {

	if (fd < 0) SETERRNO(-ENOTCONN, WM_NULL);

	wm_message_t *message = (wm_message_t *)reqbuf;

	message->type = WM_REQUEST;
	message->size = sizeof(wm_message_t);
	message->function = WM_FUNCTION_CREATE_WINDOW;

	if (send_message(message, sizeof(wm_message_t)) < 0)
		return WM_NULL;

	wm_message_t *response = (wm_message_t *)rspbuf;
	return response->result;
}

/* destroy window */
extern void wm_destroy_window(uint32_t id) {

	if (fd < 0) return;

	wm_destroy_window_request_t *message = (wm_destroy_window_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_destroy_window_request_t);
	message->base.function = WM_FUNCTION_DESTROY_WINDOW;
	message->id = id;

	send_message(&message->base, sizeof(wm_destroy_window_request_t));
}

/* set window attributes */
extern int wm_set_window_attributes(uint32_t id, uint32_t mask, wm_window_attributes_t *attributes) {

	if (fd < 0) SETERRNO(-ENOTCONN, -1);

	wm_set_window_attributes_request_t *message = (wm_set_window_attributes_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_set_window_attributes_request_t);
	message->base.function = WM_FUNCTION_SET_WINDOW_ATTRIBUTES;
	message->id = id;
	message->mask = mask;
	memcpy(&message->attributes, attributes, sizeof(message->attributes));

	return send_message(&message->base, sizeof(wm_set_window_attributes_request_t));
}

/* get window attributes */
extern int wm_get_window_attributes(uint32_t id, uint32_t mask, wm_window_attributes_t *attributes) {

	if (fd < 0) SETERRNO(-ENOTCONN, -1);

	wm_get_window_attributes_request_t *message = (wm_get_window_attributes_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_get_window_attributes_request_t);
	message->base.function = WM_FUNCTION_GET_WINDOW_ATTRIBUTES;
	message->id = id;
	message->mask = mask;

	if (send_message(&message->base, sizeof(wm_get_window_attributes_request_t)) < 0)
		return -1;

	wm_get_window_attributes_response_t *response = (wm_get_window_attributes_response_t *)rspbuf;
	memcpy(attributes, &response->attributes, sizeof(wm_window_attributes_t));

	return 0;
}

/* get queued window events */
extern wm_event_t *wm_get_queued_window_events(uint32_t id, uint32_t *count) {

	if (fd < 0) SETERRNO(-ENOTCONN, NULL);

	wm_get_queued_window_events_request_t *message = (wm_get_queued_window_events_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_get_queued_window_events_request_t);
	message->base.function = WM_FUNCTION_GET_QUEUED_WINDOW_EVENTS;
	message->id = id;

	if (send_message(&message->base, sizeof(wm_get_queued_window_events_request_t)) < 0)
		return NULL;

	wm_get_queued_window_events_response_t *response = (wm_get_queued_window_events_response_t *)rspbuf;
	*count = response->returned;

	return response->events;
}

/* post window */
extern int wm_post_window(uint32_t id) {

	if (fd < 0) SETERRNO(-ENOTCONN, -1);

	wm_post_window_request_t *message = (wm_post_window_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_post_window_request_t);
	message->base.function = WM_FUNCTION_POST_WINDOW;
	message->id = id;

	return send_message(&message->base, sizeof(wm_post_window_request_t));
}

/* draw batch */
extern int wm_draw_batch(uint32_t id, uint32_t count, wm_draw_command_t *commands) {

	if (fd < 0) SETERRNO(-ENOTCONN, -1);

	wm_draw_batch_request_t *message = (wm_draw_batch_request_t *)reqbuf;

	message->base.type = WM_REQUEST;
	message->base.size = sizeof(wm_draw_batch_request_t) + count * sizeof(wm_draw_command_t);
	message->base.function = WM_FUNCTION_DRAW_BATCH;
	message->id = id;
	message->count = count;
	memcpy(&message->commands, commands, (size_t)(count * sizeof(wm_draw_command_t)));

	return send_message(&message->base, (size_t)message->base.size);
}
