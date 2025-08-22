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
#include <crepe/title.h>

#define TEXT_MARGIN 5
#define HEIGHT 17

static const int lines[4] = {
	98, 8, 150, 7,
};
static crepe_button_style_t bstyles[CREPE_TITLE_BUTTON_COUNT] = {
	CREPE_BUTTON_STYLE_CLOSE,
	CREPE_BUTTON_STYLE_HIDE,
};

#define BG_COLOR CREPE_COLOR(0xa2, 0xa8, 0xbf)

/* title bar operations */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static void position(crepe_widget_t *widget);
static bool process_event(crepe_widget_t *widget, wm_event_t *event);
static void set_draw(crepe_widget_t *widget);
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void destroy(crepe_widget_t *widget);

crepe_widget_ops_t crepe_widget_ops_title = {
	.size = sizeof(crepe_title_t),
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

	crepe_title_t *title = CREPE_TITLE(widget);

	crepe_widget_minimum_size(title->label, dc);
	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++)
		crepe_widget_minimum_size(title->buttons[i], dc);

	widget->mwidth = TEXT_MARGIN * 2 + title->label->width;
	widget->mheight = HEIGHT;
	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++) {
		if (title->buttons[i])
			widget->mwidth += title->buttons[i]->width + (HEIGHT - title->buttons[i]->width) / 2;
	}
}

/* calculate final size */
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	crepe_title_t *title = CREPE_TITLE(widget);

	widget->width = bwidth;
	widget->height = widget->mheight;

	crepe_widget_final_size(title->label, dc, widget->width, widget->height);
	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++)
		crepe_widget_final_size(title->buttons[i], dc, widget->width, widget->height);

	/* position widgets */
	title->label->x = ((int)widget->width - (int)title->label->width) / 2;
	title->label->y = ((int)widget->height - (int)title->label->height) / 2;

	if (title->buttons[CREPE_TITLE_BUTTON_CLOSE]) {

		crepe_widget_t *button = title->buttons[CREPE_TITLE_BUTTON_CLOSE];

		button->x = (HEIGHT - (int)button->width) / 2;
		button->y = (HEIGHT - (int)button->height) / 2;
	}

	if (title->buttons[CREPE_TITLE_BUTTON_SHOW_HIDE]) {

		crepe_widget_t *button = title->buttons[CREPE_TITLE_BUTTON_SHOW_HIDE];

		button->x = (int)widget->width - ((HEIGHT + (int)button->width) / 2);
		button->y = (HEIGHT - (int)button->height) / 2;
	}
}

/* calculate absolute position */
static void position(crepe_widget_t *widget) {

	crepe_title_t *title = CREPE_TITLE(widget);

	crepe_widget_position(title->label, widget);
	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++)
		crepe_widget_position(title->buttons[i], widget);
}

/* process event */
static bool process_event(crepe_widget_t *widget, wm_event_t *event) {

	crepe_title_t *title = CREPE_TITLE(widget);
	bool prop = true;

	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++) {
		if (!crepe_widget_process_event(title->buttons[i], event)) {

			prop = false;
			break;
		}
	}
	return prop;
}

/* set needs to draw flag */
static void set_draw(crepe_widget_t *widget) {

	crepe_title_t *title = CREPE_TITLE(widget);

	crepe_widget_set_draw(title->label);
	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++)
		crepe_widget_set_draw(title->buttons[i]);
}

/* draw widget */
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_title_t *title = CREPE_TITLE(widget);

	if (widget->draw) {

		int lx = 0, rx = 0;
		if (title->buttons[CREPE_TITLE_BUTTON_CLOSE])
			lx += title->buttons[CREPE_TITLE_BUTTON_CLOSE]->x + (int)title->buttons[CREPE_TITLE_BUTTON_CLOSE]->width;
		if (title->buttons[CREPE_TITLE_BUTTON_SHOW_HIDE])
			rx += (int)widget->width - title->buttons[CREPE_TITLE_BUTTON_SHOW_HIDE]->x;

		crepe_draw_context_position(dc, widget->absx, widget->absy);
		crepe_draw_context_fill_rect(dc, widget->width, widget->height, BG_COLOR);
		crepe_draw_context_position(dc, widget->absx+TEXT_MARGIN+lx, widget->absy+(HEIGHT-lines[3])/2);
		crepe_draw_context_image_repeated(dc, dc->ui, lines[0], lines[1], (size_t)lines[2], (size_t)lines[3], (size_t)(title->label->x - lx) - TEXT_MARGIN*2, (size_t)lines[3]);
		crepe_draw_context_position(dc, title->label->absx+(int)title->label->width+TEXT_MARGIN, widget->absy+(HEIGHT-lines[3])/2);
		crepe_draw_context_image_repeated(dc, dc->ui, lines[0], lines[1], (size_t)lines[2], (size_t)lines[3], (size_t)(title->label->x - rx) - TEXT_MARGIN*2, (size_t)lines[3]);
	}

	crepe_widget_draw(title->label, dc);
	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++)
		crepe_widget_draw(title->buttons[i], dc);
}

/* destroy widget */
static void destroy(crepe_widget_t *widget) {

	crepe_title_t *title = CREPE_TITLE(widget);

	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++)
		crepe_widget_free(title->buttons[i]);
	crepe_widget_free(title->label);
}

/* create title bar */
extern crepe_widget_t *crepe_title_new(const char *text, crepe_title_button_t buttons) {

	crepe_title_t *title = CREPE_TITLE(crepe_widget_new(&crepe_widget_ops_title));

	title->label = crepe_label_new(CREPE_TEXT_STYLE_BOLD | CREPE_TEXT_STYLE_INVERTED, text);

	/* create buttons */
	for (crepe_title_button_t i = 0; i < CREPE_TITLE_BUTTON_COUNT; i++) {
		if (buttons & (1 << i))
			title->buttons[i] = crepe_button_new(bstyles[i]);
		else title->buttons[i] = NULL;
	}

	return CREPE_WIDGET(title);
}

/* set pressed callback */
extern void crepe_title_pressed(crepe_title_t *title, crepe_title_button_t button, void (*pressed)(crepe_widget_t *), void *userdata) {

	if (!title->buttons[button]) return;

	CREPE_BUTTON(title->buttons[button])->pressed = pressed;
	title->buttons[button]->userdata = userdata;
}
