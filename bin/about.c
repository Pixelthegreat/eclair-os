#include <stdio.h>
#include <stdlib.h>
#include <ec.h>
#include <crepe.h>

static crepe_context_t context = CREPE_CONTEXT_INIT; /* ui context */
static crepe_widget_t *window; /* main window */
static crepe_widget_t *title; /* title bar */
static crepe_widget_t *margin; /* margin container */
static crepe_widget_t *box; /* content box */
static crepe_widget_t *imglogo; /* logo image */
static crepe_widget_t *lblheader; /* header label */
static crepe_widget_t *lblinfo; /* kinfo label */
static crepe_widget_t *btnokay; /* okay button */

/* close pressed */
static void close_pressed(crepe_widget_t *widget) {

	crepe_window_close(CREPE_WINDOW(window));
}

/* run application */
static int run(void) {

	if (crepe_context_init(&context) != CREPE_RESULT_SUCCESS)
		return 1;

	window = crepe_window_new(&context, 0, 0);
	if (!window) return 1;

	/* get kinfo */
	ec_kinfo_t sysinfo;
	ec_kinfo(&sysinfo);

	char buf[128];
	snprintf(buf, 128, "OS name: %s\nVersion: %hhu.%hhu.%hhu\nMemory usage: %uK/%uK", sysinfo.name, sysinfo.version[0], sysinfo.version[1], sysinfo.version[2], (uint32_t)(sysinfo.mem_total - sysinfo.mem_free) >> 10, (uint32_t)sysinfo.mem_total >> 10);

	/* create widgets */
	title = crepe_title_new("About System", CREPE_TITLE_BUTTON_CLOSE_BIT);
	crepe_title_pressed(CREPE_TITLE(title), CREPE_TITLE_BUTTON_CLOSE, close_pressed, NULL);

	margin = crepe_margin_new(5, 5, 6, 6);
	box = crepe_box_new(CREPE_BOX_ORIENTATION_VERTICAL);
	box->valign = CREPE_WIDGET_ALIGN_END;

	imglogo = crepe_image_new_from_file("/usr/share/logo1.rbn");

	lblheader = crepe_label_new(CREPE_TEXT_STYLE_BOLD, "== About System ==");
	lblinfo = crepe_label_new(CREPE_TEXT_STYLE_NORMAL, buf);

	btnokay = crepe_button_new_with_label(CREPE_BUTTON_STYLE_NORMAL, "Okay");
	btnokay->halign = CREPE_WIDGET_ALIGN_END;
	CREPE_BUTTON(btnokay)->pressed = close_pressed;

	/* pack widgets */
	crepe_box_item(CREPE_BOX(box), imglogo);
	crepe_box_item(CREPE_BOX(box), lblheader);
	crepe_box_item(CREPE_BOX(box), lblinfo);
	crepe_box_item(CREPE_BOX(box), btnokay);

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
