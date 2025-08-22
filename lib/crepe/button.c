/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <crepe/core.h>
#include <crepe/label.h>
#include <crepe/button.h>

/* button style attributes */
static size_t stylesizes[] = {
	71, 21, /* normal */
	13, 13, /* close */
	13, 13, /* show */
	13, 13, /* hide */
	48, 32, /* icon */
};
static int styleposes[CREPE_BUTTON_STATE_COUNT][CREPE_BUTTON_STYLE_COUNT * 2] = {
	{
		176, 225, /* normal */
		5, 5, /* close */
		20, 5, /* show */
		35, 5, /* hide */
		126, 214, /* icon */
	}, /* normal */
	{
		176, 202, /* normal */
		50, 5, /* close */
		65, 5, /* show */
		80, 5, /* hide */
		76, 214, /* icon */
	}, /* pressed */
};

/* button operations */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static void position(crepe_widget_t *widget);
static bool process_event(crepe_widget_t *widget, wm_event_t *event);
static void set_draw(crepe_widget_t *widget);
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void destroy(crepe_widget_t *widget);

crepe_widget_ops_t crepe_widget_ops_button = {
	.size = sizeof(crepe_button_t),
	.minimum_size = minimum_size,
	.final_size = final_size,
	.position = position,
	.process_event = process_event,
	.set_draw = set_draw,
	.draw = draw,
	.destroy = destroy,
};

/* calculate minimum size */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_button_t *button = CREPE_BUTTON(widget);

	widget->mwidth = stylesizes[button->style * 2];
	widget->mheight = stylesizes[button->style * 2 + 1];

	crepe_widget_minimum_size(button->child, dc);
}

/* calculate final size */
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	crepe_button_t *button = CREPE_BUTTON(widget);

	widget->width = widget->mwidth;
	widget->height = widget->mheight;

	if (button->child) {
		
		crepe_widget_final_size(button->child, dc, widget->width, widget->height);

		button->child->x = ((int)button->child->halign * ((int)widget->width - (int)button->child->width)) / 2;
		button->child->y = ((int)button->child->valign * ((int)widget->height - (int)button->child->height)) / 2;
	}
}

/* calculate absolute position */
static void position(crepe_widget_t *widget) {

	crepe_button_t *button = CREPE_BUTTON(widget);
	crepe_widget_position(button->child, widget);
}

/* process event */
static bool process_event(crepe_widget_t *widget, wm_event_t *event) {

	crepe_button_t *button = CREPE_BUTTON(widget);
	bool prop = true;

	switch (event->type) {
		case WM_EVENT_BUTTON:
			/* button press */
			if (event->button.action == WM_ACTION_PRESSED) {

				if (event->button.position.x >= widget->absx &&
				    event->button.position.x < widget->absx + (int)widget->width &&
				    event->button.position.y >= widget->absy &&
				    event->button.position.y < widget->absy + (int)widget->height) {

					prop = false;
					button->state = CREPE_BUTTON_STATE_PRESSED;
					if (button->pressed)
						button->pressed(widget);
				}
			}
			/* button release */
			else if (button->state == CREPE_BUTTON_STATE_PRESSED) {

				prop = false;
				button->state = CREPE_BUTTON_STATE_NORMAL;
			}
			break;
	}

	if (!prop) crepe_widget_set_draw(widget);
	return prop;
}

/* set needs to draw flag */
static void set_draw(crepe_widget_t *widget) {

	crepe_button_t *button = CREPE_BUTTON(widget);
	crepe_widget_set_draw(button->child);
}

/* draw widget */
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_button_t *button = CREPE_BUTTON(widget);

	if (widget->draw) {

		crepe_draw_context_position(dc, widget->absx, widget->absy);
		crepe_draw_context_image(dc, dc->ui, styleposes[button->state][button->style * 2], styleposes[button->state][button->style * 2 + 1], widget->width, widget->height);
	}

	/* draw child */
	if (button->child && button->state == CREPE_BUTTON_STATE_PRESSED) {

		button->child->absx++;
		button->child->absy++;
	}
	crepe_widget_draw(button->child, dc);
	if (button->child && button->state == CREPE_BUTTON_STATE_PRESSED) {

		button->child->absx--;
		button->child->absy--;
	}
}

/* destroy widget */
static void destroy(crepe_widget_t *widget) {

	crepe_button_t *button = CREPE_BUTTON(widget);
	crepe_widget_free(button->child);
}

/* create button */
extern crepe_widget_t *crepe_button_new(crepe_button_style_t style) {

	crepe_button_t *button = CREPE_BUTTON(crepe_widget_new(&crepe_widget_ops_button));

	button->style = style;
	button->child = NULL;
	button->state = CREPE_BUTTON_STATE_NORMAL;
	button->pressed = NULL;

	return CREPE_WIDGET(button);
}

/* create button with label */
extern crepe_widget_t *crepe_button_new_with_label(crepe_button_style_t style, const char *text) {

	crepe_widget_t *widget = crepe_button_new(style);
	CREPE_BUTTON(widget)->child = crepe_label_new(CREPE_TEXT_STYLE_BOLD, text);

	return widget;
}

/* set button child */
extern void crepe_button_child(crepe_button_t *button, crepe_widget_t *child) {

	button->child = child;
}
