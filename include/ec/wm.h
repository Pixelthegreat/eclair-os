/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Common window manager protocol definitions
 */
#ifndef EC_WM_H
#define EC_WM_H

#include <stdint.h>
#include <ec/keycode.h>

/*
 * == Miscellaneous definitions ==
 */
#define WM_NULL 0

#define WM_CHNL 5

/*
 * == Base message structures and definitions ==
 *
 * Messages can either be requests from the client, or
 * responses from the server.
 *
 * If the message is a request, then the contents of
 * the message are based around the associated request
 * structure. If it is a response, then the contents
 * are instead based on the associated response
 * structure. If there is no associated request or
 * response structure, then the base wm_message_t
 * structure is used directly.
 */
#define WM_REQUEST 1
#define WM_RESPONSE 2

#define WM_FAILURE 0
#define WM_SUCCESS 1

typedef struct wm_message {
	uint32_t type; /* message type (request, response) */
	uint32_t size; /* message size, including header */
	union {
		uint32_t function; /* message function */
		uint32_t result; /* function result */
	};
} wm_message_t;

/*
 * == Miscellaneous functions ==
 */
#define WM_RESOURCE_IMAGE 0x2
#define WM_RESOURCE_WINDOW 0x3

/*
 * Acknowledge the existence of a client.
 *
 * - Has no arguments
 * - Should be sent after a client connects
 */
#define WM_FUNCTION_ACKNOWLEDGE 0x001

/*
 * Forget the existence of a client.
 *
 * - Has no arguments
 * - The window manager should clean up any resources
 *   that the client left behind
 * - Should be sent before a client disconnects
 */
#define WM_FUNCTION_FORGET 0x002

/*
 * == Events ==
 */
#define WM_EVENT_POST 0x1

#define WM_EVENT_KEY 0x11
#define WM_EVENT_BUTTON 0x12
#define WM_EVENT_MOTION 0x13

#define WM_EVENT_POST_BIT 0x1
#define WM_EVENT_KEY_BIT 0x10000
#define WM_EVENT_BUTTON_BIT 0x20000
#define WM_EVENT_MOTION_BIT 0x40000

#define WM_ACTION_PRESSED 1
#define WM_ACTION_RELEASED 2

typedef struct {
	uint32_t type; /* event type */
	union {
		struct {
			uint32_t code; /* key code (ec/keycode.h) */
			uint32_t action; /* associated action */
		} key; /* key press/release event */
		struct {
			uint32_t code; /* button code (ec/keycode.h) */
			uint32_t action; /* associated action */
			struct { uint32_t x, y; } position; /* relative position to window */
		} button; /* mouse button press/release event */
		struct {
			struct { int32_t x, y; } move; /* mouse position relative to previous position */
			struct { uint32_t x, y; } position; /* relative position to window */
		} motion; /* mouse motion event */
	};
} wm_event_t;

/*
 * == Image functions ==
 */

/*
 * Create an image.
 *
 * - The created image's usage is determined by its
 *   class; 'Window' images can be used as window
 *   contents and 'image' images can be drawn onto
 *   other images
 * - Should return the image resource ID on success or
 *   WM_NULL on error
 */
#define WM_FUNCTION_CREATE_IMAGE 0x101

#define WM_CLASS_IMAGE 1
#define WM_CLASS_WINDOW 2

typedef struct wm_create_image_request {
	wm_message_t base;
	uint32_t width, height; /* image size */
	uint32_t cls; /* image class */
} wm_create_image_request_t;

/*
 * Destroy a image.
 *
 * - Should return WM_SUCCESS on success or WM_FAILURE
 *   on error
 */
#define WM_FUNCTION_DESTROY_IMAGE 0x102

typedef struct wm_destroy_image_request {
	wm_message_t base;
	uint32_t id; /* image resource id */
} wm_destroy_image_request_t;

/*
 * Resize an image.
 *
 * - Should return WM_SUCCESS on success or WM_FAILURE
 *   on error
 * - This will invalidate any data currently in the
 *   image
 */
#define WM_FUNCTION_RESIZE_IMAGE 0x103

typedef struct wm_resize_image_request {
	wm_message_t base;
	uint32_t id; /* image resource id */
	uint32_t width, height; /* new image size */
} wm_resize_image_request_t;

/*
 * Set image data.
 *
 * - Should return WM_SUCCESS on success or WM_FAILURE
 *   on error
 * - Copies the provided image data to the image at the
 *   specified offset
 */
#define WM_FUNCTION_SET_IMAGE_DATA 0x104

#define WM_FORMAT_RGB8 1
#define WM_FORMAT_RGBA8 2

typedef struct wm_set_image_data_request {
	wm_message_t base;
	uint32_t id; /* image resource id */
	uint32_t format; /* data format */
	uint32_t offset; /* linear pixel offset into the image */
	uint32_t size; /* data size in pixels */
	uint8_t data[]; /* image data */
} wm_set_image_data_request_t;

/*
 * == Window functions ==
 */

/*
 * Create a window.
 *
 * - The created window has no configuration or image
 *   yet associated with it
 * - Should return the window resource ID on success or
 *   WM_NULL on error
 */
#define WM_FUNCTION_CREATE_WINDOW 0x301

/*
 * Destroy a window.
 *
 * - Should return WM_SUCCESS on success or WM_FAILURE
 *   on error
 */
#define WM_FUNCTION_DESTROY_WINDOW 0x302

typedef struct wm_destroy_window_request {
	wm_message_t base;
	uint32_t id; /* window resource id */
} wm_destroy_window_request_t;

/*
 * Configure a window/set window attributes.
 *
 * - The size parameters are the event area size and
 *   are not associated with the image size
 * - Should return WM_SUCCESS on success or WM_FAILURE
 *   on error
 */
#define WM_FUNCTION_SET_WINDOW_ATTRIBUTES 0x303

#define WM_WINDOW_ATTRIBUTE_POSITION 0x1
#define WM_WINDOW_ATTRIBUTE_SIZE 0x2
#define WM_WINDOW_ATTRIBUTE_EVENTS 0x4
#define WM_WINDOW_ATTRIBUTE_IMAGE 0x8
#define WM_WINDOW_ATTRIBUTE_STACK 0x10

#define WM_WINDOW_ATTRIBUTE_ALL (~0)

#define WM_STACK_BELOW 1 /* always below */
#define WM_STACK_CENTER 2 /* normal */
#define WM_STACK_ABOVE 3 /* always above */

typedef struct wm_window_attributes {
	uint32_t x, y; /* position */
	uint32_t width, height; /* event area size */
	uint32_t events; /* event mask */
	uint32_t image; /* image resource id or WM_NULL */
	uint32_t stack; /* position in stack */
} wm_window_attributes_t;

typedef struct wm_set_window_attributes_request {
	wm_message_t base;
	uint32_t id; /* window resource id */
	uint32_t mask; /* window attribute mask */
	wm_window_attributes_t attributes; /* window attributes */
} wm_set_window_attributes_request_t;

/*
 * Get window attributes.
 *
 * - Should return WM_SUCCESS on success along with the
 *   requested window attributes or WM_FAILURE on error
 */
#define WM_FUNCTION_GET_WINDOW_ATTRIBUTES 0x304

typedef struct wm_get_window_attributes_request {
	wm_message_t base;
	uint32_t id; /* window resource id */
	uint32_t mask; /* window attribute mask */
} wm_get_window_attributes_request_t;

typedef struct wm_get_window_attributes_response {
	wm_message_t base;
	wm_window_attributes_t attributes; /* window attributes */
} wm_get_window_attributes_response_t;

/*
 * Get queued window events.
 *
 * - Should return WM_SUCCESS on success along with as
 *   many queued window events are available and can
 *   fit within the message, or WM_FAILURE on error
 */
#define WM_FUNCTION_GET_QUEUED_WINDOW_EVENTS 0x305

typedef struct wm_get_queued_window_events_request {
	wm_message_t base;
	uint32_t id; /* window resource id */
} wm_get_queued_window_events_request_t;

typedef struct wm_get_queued_window_events_response {
	wm_message_t base;
	uint32_t returned; /* returned events */
	uint32_t remaining; /* remaining events if any */
	wm_event_t events[]; /* event data */
} wm_get_queued_window_events_response_t;

/*
 * Draw window image.
 *
 * - Should return WM_SUCCESS on success or WM_FAILURE
 *   on error
 * - If the window is at the top of the stack or has
 *   WM_STACK_ABOVE, its image will be copied to the
 *   screen; Otherwise, nothing will happen, but
 *   WM_SUCCESS should still be returned
 */
#define WM_FUNCTION_POST_WINDOW 0x306

typedef struct wm_post_window_request {
	wm_message_t base;
	uint32_t id; /* window resource id */
} wm_post_window_request_t;

#endif /* EC_WM_H */
