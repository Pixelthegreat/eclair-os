#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <crepe.h>

static crepe_context_t context = CREPE_CONTEXT_INIT; /* ui context */
static crepe_widget_t *window; /* main window */
static crepe_widget_t *title; /* title bar */
static crepe_widget_t *margin; /* margin container */
static crepe_widget_t *box; /* content box */
static crepe_widget_t *button1; /* test button one */
static crepe_widget_t *button2; /* test button two */

#define WIDTH 256
#define HEIGHT 192

/* run application */
static int run(void) {

	if (crepe_context_init(&context) != CREPE_RESULT_SUCCESS)
		return 1;

	window = crepe_window_new(&context, WIDTH, HEIGHT);
	if (!window) return 1;

	/* create widgets */
	title = crepe_title_new("Window Test", CREPE_TITLE_BUTTON_CLOSE_BIT | CREPE_TITLE_BUTTON_SHOW_HIDE_BIT);

	margin = crepe_margin_new(5, 5, 6, 6);
	margin->halign = CREPE_WIDGET_ALIGN_END;
	margin->valign = CREPE_WIDGET_ALIGN_END;

	box = crepe_box_new(CREPE_BOX_ORIENTATION_HORIZONTAL);

	button1 = crepe_button_new_with_label(CREPE_BUTTON_STYLE_NORMAL, "Hello");
	button2 = crepe_button_new_with_label(CREPE_BUTTON_STYLE_NORMAL, "World");

	crepe_box_item(CREPE_BOX(box), button1);
	crepe_box_item(CREPE_BOX(box), button2);

	crepe_margin_child(CREPE_MARGIN(margin), box);

	crepe_window_title(CREPE_WINDOW(window), title);
	crepe_window_child(CREPE_WINDOW(window), margin);

	/* present window */
	crepe_window_present(CREPE_WINDOW(window));
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
