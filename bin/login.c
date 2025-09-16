#include <stdio.h>
#include <stdlib.h>
#include <ec.h>
#include <ec/wm.h>
#include <crepe.h>

#define DESKTOP_PATH "/bin/about"

static size_t width = 0, height = 0; /* screen size */

static crepe_context_t context = CREPE_CONTEXT_INIT; /* ui context */

static crepe_widget_t *bgwindow; /* background window */
static crepe_widget_t *color; /* background color */

static crepe_widget_t *window; /* main login window */
static crepe_widget_t *title; /* title bar */
static crepe_widget_t *margin; /* margin area */
static crepe_widget_t *vbox; /* vertical box */
static crepe_widget_t *label; /* welcome label */
static crepe_widget_t *userbox; /* username box */
static crepe_widget_t *userlabel; /* username label */
static crepe_widget_t *userentry; /* username entry */
static crepe_widget_t *pswdbox; /* password box */
static crepe_widget_t *pswdlabel; /* password label */
static crepe_widget_t *pswdentry; /* password entry */
static crepe_widget_t *loginbtn; /* login button */

/* close pressed */
static void close_pressed(crepe_widget_t *widget) {

	crepe_window_close(CREPE_WINDOW(window));
}

/* background window drawn */
static void bgwindow_drawn(crepe_widget_t *widget, crepe_draw_context_t *dc) {

	crepe_draw_context_clear(dc, CREPE_COLOR(0xee, 0xcc, 0xaa));
}

/* login button pressed */
static void login_pressed(crepe_widget_t *widget) {

	const char *name = CREPE_ENTRY(userentry)->text;
	const char *pswd = CREPE_ENTRY(pswdentry)->text;

	if (ec_setuser(name, pswd) < 0) {

		crepe_widget_t *message = crepe_message_box_new(&context, NULL, CREPE_MESSAGE_BUTTON_OKAY_BIT, "Login failed", "Incorrect username or password");
		return;
	}

	const char *argv[] = {DESKTOP_PATH, NULL};
	ec_pexec(DESKTOP_PATH, argv, NULL);
}

/* create login window */
static int create_login_window(void) {

	window = crepe_window_new(&context, 0, 0);
	if (!window) return -1;

	title = crepe_title_new("Login", 0);

	margin = crepe_margin_new(5, 5, 6, 6);
	vbox = crepe_box_new(CREPE_BOX_ORIENTATION_VERTICAL);

	label = crepe_label_new(CREPE_TEXT_STYLE_NORMAL, "   Welcome to Eclair OS!\nPlease log in to continue.");

	userbox = crepe_box_new(CREPE_BOX_ORIENTATION_HORIZONTAL);
	userbox->halign = CREPE_WIDGET_ALIGN_END;
	userlabel = crepe_label_new(CREPE_TEXT_STYLE_NORMAL, "Username: ");
	userentry = crepe_entry_new();

	pswdbox = crepe_box_new(CREPE_BOX_ORIENTATION_HORIZONTAL);
	pswdbox->halign = CREPE_WIDGET_ALIGN_END;
	pswdlabel = crepe_label_new(CREPE_TEXT_STYLE_NORMAL, "Password: ");
	pswdentry = crepe_entry_new();

	loginbtn = crepe_button_new_with_label(CREPE_BUTTON_STYLE_NORMAL, "Login");
	loginbtn->halign = CREPE_WIDGET_ALIGN_END;
	CREPE_BUTTON(loginbtn)->pressed = login_pressed;

	/* parent widgets */
	crepe_box_item(CREPE_BOX(userbox), userlabel);
	crepe_box_item(CREPE_BOX(userbox), userentry);

	crepe_box_item(CREPE_BOX(pswdbox), pswdlabel);
	crepe_box_item(CREPE_BOX(pswdbox), pswdentry);

	crepe_box_item(CREPE_BOX(vbox), label);
	crepe_box_item(CREPE_BOX(vbox), userbox);
	crepe_box_item(CREPE_BOX(vbox), pswdbox);
	crepe_box_item(CREPE_BOX(vbox), loginbtn);

	crepe_margin_child(CREPE_MARGIN(margin), vbox);

	crepe_window_title(CREPE_WINDOW(window), title);
	crepe_window_child(CREPE_WINDOW(window), margin);
	crepe_window_present(CREPE_WINDOW(window));

	return 0;
}

/* run application */
static int run(void) {

	if (crepe_context_init(&context) != CREPE_RESULT_SUCCESS)
		return 1;

	wm_screen_info_t scinfo;
	wm_get_screen_info(&scinfo);

	width = (size_t)scinfo.width;
	height = (size_t)scinfo.height;

	/* create background window */
	bgwindow = crepe_window_new(&context, width, height);
	if (!bgwindow) return 1;

	CREPE_WINDOW(bgwindow)->decorations = false;
	CREPE_WINDOW(bgwindow)->stack = WM_STACK_BELOW;
	CREPE_WINDOW(bgwindow)->drawn = bgwindow_drawn;

	color = crepe_label_new(CREPE_TEXT_STYLE_NORMAL, "");
	crepe_window_child(CREPE_WINDOW(bgwindow), color);

	crepe_window_present(CREPE_WINDOW(bgwindow));

	/* create login window */
	if (create_login_window() < 0)
		return 1;

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
