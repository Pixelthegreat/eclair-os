/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_MESSAGE_H
#define CREPE_MESSAGE_H

#include <crepe/core.h>
#include <crepe/widget.h>
#include <crepe/context.h>

/* message box buttons */
typedef enum crepe_message_response {
	CREPE_MESSAGE_RESPONSE_CANCEL = 0,
	CREPE_MESSAGE_RESPONSE_APPLY,
	CREPE_MESSAGE_RESPONSE_ACCEPT,

	CREPE_MESSAGE_RESPONSE_COUNT,
} crepe_message_response_t;

typedef enum crepe_message_button {
	CREPE_MESSAGE_BUTTON_OKAY = 0,
	CREPE_MESSAGE_BUTTON_CANCEL,
	CREPE_MESSAGE_BUTTON_APPLY,
	CREPE_MESSAGE_BUTTON_YES,
	CREPE_MESSAGE_BUTTON_NO,

	CREPE_MESSAGE_BUTTON_COUNT,

	CREPE_MESSAGE_BUTTON_OKAY_BIT = 0x1,
	CREPE_MESSAGE_BUTTON_CANCEL_BIT = 0x2,
	CREPE_MESSAGE_BUTTON_APPLY_BIT = 0x4,
	CREPE_MESSAGE_BUTTON_YES_BIT = 0x8,
	CREPE_MESSAGE_BUTTON_NO_BIT = 0x10,
} crepe_message_button_t;

/* functions */
extern crepe_widget_t *crepe_message_box_new(crepe_context_t *context, crepe_message_response_t *response, crepe_message_button_t buttons, const char *title, const char *message); /* create message box */

#endif /* CREPE_MESSAGE_H */
