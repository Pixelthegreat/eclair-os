/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <crepe/core.h>
#include <crepe/entry.h>

static int rect[4] = {104, 179, 143, 21};
static int cursor[4] = {3, 21, 1, 11};

/* entry operations */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc);
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight);
static bool process_event(crepe_widget_t *widget, wm_event_t *event);
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc);

crepe_widget_ops_t crepe_widget_ops_entry = {
	.size = sizeof(crepe_entry_t),
	.minimum_size = minimum_size,
	.final_size = final_size,
	.process_event = process_event,
	.draw = draw,
};

/* calculate minimum size */
static void minimum_size(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	widget->mwidth = (size_t)rect[2];
	widget->mheight = (size_t)rect[3];
}

/* calculate final size */
static void final_size(crepe_widget_t *widget, crepe_draw_context_t *dc, size_t bwidth, size_t bheight) {

	widget->width = widget->mwidth;
	widget->height = widget->mheight;
}

/* process event */
static bool process_event(crepe_widget_t *widget, wm_event_t *event) {

	crepe_entry_t *entry = CREPE_ENTRY(widget);
	bool prop = true;

	if (entry->active) {
		switch (event->type) {

			/* button press */
			case WM_EVENT_BUTTON:
				if (event->button.action != WM_ACTION_PRESSED)
					break;

				if (event->button.position.x < widget->absx ||
				    event->button.position.x >= widget->absx + (int)widget->width ||
				    event->button.position.y < widget->absy ||
				    event->button.position.y >= widget->absy + (int)widget->height) {

					entry->active = false;
					crepe_widget_set_draw(widget);
				}
				break;

			/* key press */
			case WM_EVENT_KEY:
				prop = false;

				if (event->key.action != WM_ACTION_PRESSED) {

					if (event->key.code == ECK_LEFT_SHIFT)
						entry->shift = false;
					break;
				}

				if (event->key.code == ECK_BACKSPACE) {
					if (entry->pos) {
						
						entry->text[--entry->pos] = 0;
						crepe_widget_set_draw(widget);
					}
				}
				else if (event->key.code == ECK_LEFT_SHIFT)
					entry->shift = true;

				else if (entry->pos < CREPE_ENTRY_MAX_CHARS) {

					int ch = eck_aschar(event->key.code);
					if (!ch) break;

					if (entry->shift) ch = toupper(ch);

					entry->text[entry->pos++] = (char)ch;
					crepe_widget_set_draw(widget);
				}
				break;
		}
	}

	/* set active */
	else if (event->type == WM_EVENT_BUTTON &&
		 event->button.action == WM_ACTION_PRESSED &&
		 event->button.position.x >= widget->absx &&
		 event->button.position.x < widget->absx + (int)widget->width &&
		 event->button.position.y >= widget->absy &&
		 event->button.position.y < widget->absy + (int)widget->height) {

		entry->active = true;
		crepe_widget_set_draw(widget);
	}
	return prop;
}

/* draw widget */
static void draw(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_entry_t *entry = CREPE_ENTRY(widget);

	if (widget->draw) {

		crepe_draw_context_position(dc, widget->absx, widget->absy);
		crepe_draw_context_image(dc, dc->ui, rect[0], rect[1], widget->width, widget->height);

		int w, h;
		crepe_draw_context_text_size(dc, CREPE_TEXT_STYLE_NORMAL, entry->text, &w, &h);

		int margin = ((int)widget->height - h) / 2;
		crepe_draw_context_position(dc, widget->absx + margin, widget->absy + (int)margin);
		crepe_draw_context_text(dc, CREPE_TEXT_STYLE_NORMAL, entry->text);

		/* draw text cursor */
		if (entry->active) {

			crepe_draw_context_position(dc, widget->absx + margin + w + 1, widget->absy + (int)margin);
			crepe_draw_context_image(dc, dc->ui, cursor[0], cursor[1], (size_t)cursor[2], (size_t)cursor[3]);
		}
	}
}

/* create entry */
extern crepe_widget_t *crepe_entry_new(void) {

	crepe_entry_t *entry = CREPE_ENTRY(crepe_widget_new(&crepe_widget_ops_entry));

	entry->active = false;
	entry->shift = false;
	entry->text[0] = 0;
	entry->pos = 0;

	return CREPE_WIDGET(entry);
}
