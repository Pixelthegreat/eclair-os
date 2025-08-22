/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_BUTTON_H
#define CREPE_BUTTON_H

#include <crepe/core.h>
#include <crepe/widget.h>

/* button style */
typedef enum crepe_button_style {
	CREPE_BUTTON_STYLE_NORMAL = 0,
	CREPE_BUTTON_STYLE_CLOSE,
	CREPE_BUTTON_STYLE_SHOW,
	CREPE_BUTTON_STYLE_HIDE,
	CREPE_BUTTON_STYLE_ICON,

	CREPE_BUTTON_STYLE_COUNT,
} crepe_button_style_t;

/* button state */
typedef enum crepe_button_state {
	CREPE_BUTTON_STATE_NORMAL = 0,
	CREPE_BUTTON_STATE_PRESSED,

	CREPE_BUTTON_STATE_COUNT,
} crepe_button_state_t;

/* button */
typedef struct crepe_button {
	crepe_widget_t base;
	crepe_button_style_t style; /* button style */
	crepe_widget_t *child; /* child widget */
	crepe_button_state_t state; /* button state */
	void (*pressed)(crepe_widget_t *); /* pressed callback */
} crepe_button_t;

#define CREPE_BUTTON(p) ((crepe_button_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_button;

/* functions */
extern crepe_widget_t *crepe_button_new(crepe_button_style_t style); /* create button */
extern crepe_widget_t *crepe_button_new_with_label(crepe_button_style_t style, const char *text); /* create button with label */
extern void crepe_button_child(crepe_button_t *button, crepe_widget_t *child); /* set button child */

#endif /* CREPE_BUTTON_H */
