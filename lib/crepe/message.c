/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <crepe/core.h>
#include <crepe/title.h>
#include <crepe/box.h>
#include <crepe/label.h>
#include <crepe/button.h>
#include <crepe/margin.h>
#include <crepe/message.h>

static const char *text[] = {
	"Okay",
	"Cancel",
	"Apply",
	"Yes",
	"No",
};
static crepe_message_response_t responses[] = {
	CREPE_MESSAGE_RESPONSE_ACCEPT,
	CREPE_MESSAGE_RESPONSE_CANCEL,
	CREPE_MESSAGE_RESPONSE_APPLY,
	CREPE_MESSAGE_RESPONSE_ACCEPT,
	CREPE_MESSAGE_RESPONSE_CANCEL,
};

/* close pressed */
static void close_pressed(crepe_widget_t *widget) {

	crepe_widget_t *window = CREPE_WIDGET(widget->userdata);

	if (window->userdata) {

		crepe_message_response_t response = CREPE_MESSAGE_RESPONSE_CANCEL;
		if (CREPE_BUTTON(widget)->child)
			response = responses[(crepe_message_button_t)CREPE_BUTTON(widget)->child->userdata];

		*(crepe_message_response_t *)window->userdata = response;
	}

	crepe_window_close(CREPE_WINDOW(window));
}

/* create message box */
extern crepe_widget_t *crepe_message_box_new(crepe_context_t *context, crepe_message_response_t *response, crepe_message_button_t buttons, const char *title, const char *message) {

	crepe_widget_t *window = crepe_window_new(context, 0, 0);
	if (!window) return NULL;

	window->userdata = response;

	crepe_widget_t *wtitle = crepe_title_new(title, CREPE_TITLE_BUTTON_CLOSE_BIT);
	crepe_title_pressed(CREPE_TITLE(wtitle), CREPE_TITLE_BUTTON_CLOSE, close_pressed, window);

	crepe_widget_t *margin = crepe_margin_new(5, 5, 6, 6);
	crepe_widget_t *box = crepe_box_new(CREPE_BOX_ORIENTATION_VERTICAL);

	crepe_widget_t *inner = crepe_margin_new(10, 10, 10, 10);
	crepe_widget_t *label = crepe_label_new(CREPE_TEXT_STYLE_NORMAL, message);

	/* create buttons */
	crepe_widget_t *hbox = crepe_box_new(CREPE_BOX_ORIENTATION_HORIZONTAL);
	hbox->halign = CREPE_WIDGET_ALIGN_END;

	for (crepe_message_button_t i = 0; i < CREPE_MESSAGE_BUTTON_COUNT; i++) {

		if (buttons & (1 << i)) {

			crepe_widget_t *button = crepe_button_new_with_label(CREPE_BUTTON_STYLE_NORMAL, text[i]);
			button->userdata = window;
			CREPE_BUTTON(button)->pressed = close_pressed;
			CREPE_BUTTON(button)->child->userdata = (void *)i;

			crepe_box_item(CREPE_BOX(hbox), button);
		}
	}

	/* pack widgets */
	crepe_margin_child(CREPE_MARGIN(inner), label);

	crepe_box_item(CREPE_BOX(box), inner);
	crepe_box_item(CREPE_BOX(box), hbox);

	crepe_margin_child(CREPE_MARGIN(margin), box);

	crepe_window_title(CREPE_WINDOW(window), wtitle);
	crepe_window_child(CREPE_WINDOW(window), margin);
	crepe_window_present(CREPE_WINDOW(window));

	return window;
}
