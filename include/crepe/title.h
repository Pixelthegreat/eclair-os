/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_TITLE_H
#define CREPE_TITLE_H

#include <crepe/core.h>
#include <crepe/widget.h>

/* title bar buttons */
typedef enum crepe_title_button {
	CREPE_TITLE_BUTTON_CLOSE = 0,
	CREPE_TITLE_BUTTON_SHOW_HIDE,

	CREPE_TITLE_BUTTON_COUNT,

	CREPE_TITLE_BUTTON_CLOSE_BIT = 0x1,
	CREPE_TITLE_BUTTON_SHOW_HIDE_BIT = 0x2,
} crepe_title_button_t;

/* title bar */
typedef struct crepe_title {
	crepe_widget_t base;
	crepe_widget_t *label; /* title label */
	crepe_widget_t *buttons[CREPE_TITLE_BUTTON_COUNT]; /* title bar buttons */
} crepe_title_t;

#define CREPE_TITLE(p) ((crepe_title_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_title;

/* functions */
extern crepe_widget_t *crepe_title_new(const char *text, crepe_title_button_t buttons); /* create title bar */
extern void crepe_title_pressed(crepe_title_t *title, crepe_title_button_t button, void (*pressed)(crepe_widget_t *), void *userdata); /* set pressed callback */

#endif /* CREPE_TITLE_H */
