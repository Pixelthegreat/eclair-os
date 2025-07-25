/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ec.h>
#include <wm/input.h>
#include <wm/screen.h>
#include <wm/window.h>
#include <wm/server.h>

int main() {

	if (input_init() < 0)
		return 1;
	if (screen_init() < 0)
		return 1;
	if (server_host() < 0)
		return 1;

	/* create test window */
	resource_t *window = window_resource_new();
	if (!window) return 1;

	resource_t *rimage = image_resource_new(WM_CLASS_WINDOW, 512, 384);
	if (!rimage) return 1;

	image_t *image = image_resource_get_image(rimage);
	color_t color = {0xaa, 0xcc, 0xee};
	image_fill(image, color);

	wm_window_attributes_t attributes = {
		.x = 24, .y = 24,
		.width = 512, .height = 384,
		.events = 0,
		.image = rimage->id,
		.stack = WM_STACK_CENTER,
	};
	window_resource_set_attributes(window, &attributes, WM_WINDOW_ATTRIBUTE_ALL);

	window = window_resource_new();
	if (!window) return 1;

	rimage = image_resource_new(WM_CLASS_WINDOW, 512, 384);
	if (!rimage) return 1;

	image = image_resource_get_image(rimage);
	color = (color_t){0xcc, 0xaa, 0xee};
	image_fill(image, color);

	attributes.x = 240;
	attributes.y = 180;
	attributes.image = rimage->id;

	window_resource_set_attributes(window, &attributes, WM_WINDOW_ATTRIBUTE_ALL);

	while (true) server_update();
}
