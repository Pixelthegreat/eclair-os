/*
 * Common key codes (and mouse button codes) to be used between the kernel and libraries
 */
#ifndef EC_KEYCODE_H
#define EC_KEYCODE_H

/* key codes */
typedef enum ec_keycode {
	ECK_NONE = 0,

	ECK_0,
	ECK_1,
	ECK_2,
	ECK_3,
	ECK_4,
	ECK_5,
	ECK_6,
	ECK_7,
	ECK_8,
	ECK_9,

	ECK_A, /* 1 */
	ECK_B, /* 2 */
	ECK_C, /* 3 */
	ECK_D, /* 4 */
	ECK_E, /* 5 */
	ECK_F, /* 6 */
	ECK_G, /* 7 */
	ECK_H, /* 8 */
	ECK_I, /* 9 */
	ECK_J, /* 10 */
	ECK_K, /* 11 */
	ECK_L, /* 12 */
	ECK_M, /* 13 */
	ECK_N, /* 14 */
	ECK_O, /* 15 */
	ECK_P, /* 16 */
	ECK_Q, /* 17 */
	ECK_R, /* 18 */
	ECK_S, /* 19 */
	ECK_T, /* 20 */
	ECK_U, /* 21 */
	ECK_V, /* 22 */
	ECK_W, /* 23 */
	ECK_X, /* 24 */
	ECK_Y, /* 25 */
	ECK_Z, /* 26 */

	ECK_BACKTICK,
	ECK_SEMICOLON,
	ECK_QUOTE,
	ECK_LEFT_PAREN,
	ECK_RIGHT_PAREN,
	ECK_LEFT_BRACKET,
	ECK_RIGHT_BRACKET,
	ECK_SLASH,
	ECK_BACKSLASH,
	ECK_COMMA,
	ECK_DOT,
	ECK_EQUALS,
	ECK_MINUS,

	ECK_SPACE,
	ECK_BACKSPACE,
	ECK_LEFT_SHIFT,
	ECK_RIGHT_SHIFT,
	ECK_LEFT_CONTROL,
	ECK_RIGHT_CONTROL,
	ECK_LEFT_ALT,
	ECK_RIGHT_ALT,
	ECK_CAPS_LOCK,
	ECK_NUM_LOCK,
	ECK_TAB,
	ECK_ESCAPE,
	ECK_RETURN,

	ECK_PRINT_SCREEN,
	ECK_INSERT,
	ECK_DELETE,
	ECK_HOME,
	ECK_END,
	ECK_PAGE_UP,
	ECK_PAGE_DOWN,

	ECK_LEFT,
	ECK_RIGHT,
	ECK_UP,
	ECK_DOWN,

	ECK_NUMPAD_0,
	ECK_NUMPAD_1,
	ECK_NUMPAD_2,
	ECK_NUMPAD_3,
	ECK_NUMPAD_4,
	ECK_NUMPAD_5,
	ECK_NUMPAD_6,
	ECK_NUMPAD_7,
	ECK_NUMPAD_8,
	ECK_NUMPAD_9,
	ECK_NUMPAD_SLASH,
	ECK_NUMPAD_ASTERISK,
	ECK_NUMPAD_MINUS,
	ECK_NUMPAD_PLUS,
	ECK_NUMPAD_DOT,
	ECK_NUMPAD_ENTER,

	ECK_F1,
	ECK_F2,
	ECK_F3,
	ECK_F4,
	ECK_F5,
	ECK_F6,
	ECK_F7,
	ECK_F8,
	ECK_F9,
	ECK_F10,
	ECK_F11,
	ECK_F12,

	ECK_COUNT,

	ECK_CODE = 0xff,
	ECK_PRESS = 0x000,
	ECK_RELEASE = 0x100,
} ec_keycode_t;

/* button codes */
typedef enum ec_buttoncode {
	ECB_1 = 0,
	ECB_2,
	ECB_3,

	ECB_COUNT,

	ECB_LEFT = ECB_1,
	ECB_MIDDLE = ECB_2,
	ECB_RIGHT = ECB_3,
} ec_buttoncode_t;

#endif /* EC_KEYCODE_H */
