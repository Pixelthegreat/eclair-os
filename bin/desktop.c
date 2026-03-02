/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <ec.h>
#include <ec/wm.h>
#include <crepe.h>

static size_t width, height;

static crepe_context_t context = CREPE_CONTEXT_INIT;

static crepe_widget_t *panelwindow; /* panel window */
static crepe_widget_t *panelmargin; /* panel margin area */
static crepe_widget_t *panelbox; /* panel box */
static crepe_widget_t *panelmenubtn; /* panel menu button */
static crepe_widget_t *panelrunbtn; /* panel run button */

static crepe_widget_t *runwindow; /* run window */
static crepe_widget_t *runtitle; /* run window title */
static crepe_widget_t *runmargin; /* run margin area */
static crepe_widget_t *runbox; /* run main box */
static crepe_widget_t *runinner; /* run inner margin */
static crepe_widget_t *runhbox; /* run button box */

/* menu button pressed */
static void panelmenubtn_pressed(crepe_widget_t *widget) {

	crepe_widget_t *message = crepe_message_box_new(&context, NULL,
			CREPE_MESSAGE_BUTTON_YES_BIT | CREPE_MESSAGE_BUTTON_NO_BIT,
			"Are you sure?", "Are you sure you want to continue?");
}

/* run button pressed */
static void panelrunbtn_pressed(crepe_widget_t *widget) {

	if (runwindow) return;
}

/* run application */
static int run(void) {

	if (crepe_context_init(&context) != CREPE_RESULT_SUCCESS)
		return 1;

	wm_screen_info_t scinfo;
	wm_get_screen_info(&scinfo);

	width = (size_t)scinfo.width;
	height = (size_t)scinfo.height;

	/* create main panel window */
	panelwindow = crepe_window_new(&context, width, 0);
	if (!panelwindow) return 1;

	panelmargin = crepe_margin_new(1, 1, 1, 1);
	panelmargin->halign = CREPE_WIDGET_ALIGN_START;

	panelbox = crepe_box_new(CREPE_BOX_ORIENTATION_HORIZONTAL);

	panelmenubtn = crepe_button_new_with_label(CREPE_BUTTON_STYLE_MENU, "Menu");
	CREPE_BUTTON(panelmenubtn)->pressed = panelmenubtn_pressed;

	panelrunbtn = crepe_button_new_with_label(CREPE_BUTTON_STYLE_MENU, "Run...");
	CREPE_BUTTON(panelrunbtn)->pressed = panelrunbtn_pressed;

	crepe_box_item(CREPE_BOX(panelbox), panelmenubtn);
	crepe_box_item(CREPE_BOX(panelbox), panelrunbtn);

	crepe_margin_child(CREPE_MARGIN(panelmargin), panelbox);
	crepe_window_child(CREPE_WINDOW(panelwindow), panelmargin);
	crepe_window_present(CREPE_WINDOW(panelwindow));

	/* set window to corner */
	wm_window_attributes_t attributes = {
		.x = 0, .y = 0,
	};
	wm_set_window_attributes(CREPE_WINDOW(panelwindow)->window,
				 WM_WINDOW_ATTRIBUTE_POSITION,
				 &attributes);

	/* main loop */
	crepe_context_main_loop(&context);
	return 0;
}

/* clean up resources */
static void cleanup(void) {

	crepe_context_destroy(&context);
}

int main() {

	int code = run();
	cleanup();
	return code;
}
