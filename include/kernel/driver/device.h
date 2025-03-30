/* simple device manager */
#ifndef DEVICE_H
#define DEVICE_H

#include <kernel/types.h>

struct device;

/* information for device class */
#define DEVCLASS_DESC_MAX_CHARS 32

typedef struct devclass {
	size_t size; /* size of device object */
	char desc[DEVCLASS_DESC_MAX_CHARS]; /* name of device class */
	struct device *first; /* first child */
	struct device *last; /* last child */
} devclass_t;

#define DEVCLASS_INIT(psize, pdesc) {.size = psize, .desc = pdesc, .first = NULL, .last = NULL}

/* information for device */
#define DEVICE_DESC_MAX_CHARS 32

typedef struct device {
	devclass_t *cls; /* device class */
	char desc[DEVICE_DESC_MAX_CHARS]; /* device description */
	uint32_t impl; /* implementation specific value */
	bool held; /* indicate if a resource is busy */
	struct device *clsnext; /* next sibling in class */
	struct device *busnext; /* next sibling in bus */
} device_t;

/* bus device */
typedef struct device_bus {
	device_t base;
	device_t *first; /* first child */
	device_t *last; /* last child */
} device_bus_t;

/* keyboard device */
#define DEVICE_KEYBOARD_MAX_KEYS 32

typedef struct device_keyboard {
	device_t base;
	int keys[DEVICE_KEYBOARD_MAX_KEYS]; /* key info */
	int kstart, kend; /* ring buffer positions */
} device_keyboard_t;

/* keycodes */
typedef enum device_keycode {
	DEVICE_KEYCODE_NONE = 0,

	DEVICE_KEYCODE_0,
	DEVICE_KEYCODE_1,
	DEVICE_KEYCODE_2,
	DEVICE_KEYCODE_3,
	DEVICE_KEYCODE_4,
	DEVICE_KEYCODE_5,
	DEVICE_KEYCODE_6,
	DEVICE_KEYCODE_7,
	DEVICE_KEYCODE_8,
	DEVICE_KEYCODE_9,/* create terminal device (stub) */

	DEVICE_KEYCODE_A, /* 1 */
	DEVICE_KEYCODE_B, /* 2 */
	DEVICE_KEYCODE_C, /* 3 */
	DEVICE_KEYCODE_D, /* 4 */
	DEVICE_KEYCODE_E, /* 5 */
	DEVICE_KEYCODE_F, /* 6 */
	DEVICE_KEYCODE_G, /* 7 */
	DEVICE_KEYCODE_H, /* 8 */
	DEVICE_KEYCODE_I, /* 9 */
	DEVICE_KEYCODE_J, /* 10 */
	DEVICE_KEYCODE_K, /* 11 */
	DEVICE_KEYCODE_L, /* 12 */
	DEVICE_KEYCODE_M, /* 13 */
	DEVICE_KEYCODE_N, /* 14 */
	DEVICE_KEYCODE_O, /* 15 */
	DEVICE_KEYCODE_P, /* 16 */
	DEVICE_KEYCODE_Q, /* 17 */
	DEVICE_KEYCODE_R, /* 18 */
	DEVICE_KEYCODE_S, /* 19 */
	DEVICE_KEYCODE_T, /* 20 */
	DEVICE_KEYCODE_U, /* 21 */
	DEVICE_KEYCODE_V, /* 22 */
	DEVICE_KEYCODE_W, /* 23 */
	DEVICE_KEYCODE_X, /* 24 */
	DEVICE_KEYCODE_Y, /* 25 */
	DEVICE_KEYCODE_Z, /* 26 */

	DEVICE_KEYCODE_BACKTICK,
	DEVICE_KEYCODE_SEMICOLON,
	DEVICE_KEYCODE_QUOTE,
	DEVICE_KEYCODE_LEFT_PAREN,
	DEVICE_KEYCODE_RIGHT_PAREN,
	DEVICE_KEYCODE_LEFT_BRACKET,
	DEVICE_KEYCODE_RIGHT_BRACKET,
	DEVICE_KEYCODE_SLASH,
	DEVICE_KEYCODE_BACKSLASH,
	DEVICE_KEYCODE_COMMA,
	DEVICE_KEYCODE_DOT,
	DEVICE_KEYCODE_EQUALS,
	DEVICE_KEYCODE_MINUS,

	DEVICE_KEYCODE_SPACE,
	DEVICE_KEYCODE_BACKSPACE,
	DEVICE_KEYCODE_LEFT_SHIFT,
	DEVICE_KEYCODE_RIGHT_SHIFT,
	DEVICE_KEYCODE_LEFT_CONTROL,
	DEVICE_KEYCODE_RIGHT_CONTROL,
	DEVICE_KEYCODE_LEFT_ALT,
	DEVICE_KEYCODE_RIGHT_ALT,
	DEVICE_KEYCODE_CAPS_LOCK,
	DEVICE_KEYCODE_NUM_LOCK,
	DEVICE_KEYCODE_TAB,
	DEVICE_KEYCODE_ESCAPE,
	DEVICE_KEYCODE_RETURN,

	DEVICE_KEYCODE_PRINT_SCREEN,
	DEVICE_KEYCODE_INSERT,
	DEVICE_KEYCODE_DELETE,
	DEVICE_KEYCODE_HOME,
	DEVICE_KEYCODE_END,
	DEVICE_KEYCODE_PAGE_UP,
	DEVICE_KEYCODE_PAGE_DOWN,

	DEVICE_KEYCODE_LEFT,
	DEVICE_KEYCODE_RIGHT,
	DEVICE_KEYCODE_UP,
	DEVICE_KEYCODE_DOWN,

	DEVICE_KEYCODE_NUMPAD_0,
	DEVICE_KEYCODE_NUMPAD_1,
	DEVICE_KEYCODE_NUMPAD_2,
	DEVICE_KEYCODE_NUMPAD_3,
	DEVICE_KEYCODE_NUMPAD_4,
	DEVICE_KEYCODE_NUMPAD_5,
	DEVICE_KEYCODE_NUMPAD_6,
	DEVICE_KEYCODE_NUMPAD_7,
	DEVICE_KEYCODE_NUMPAD_8,
	DEVICE_KEYCODE_NUMPAD_9,
	DEVICE_KEYCODE_NUMPAD_SLASH,
	DEVICE_KEYCODE_NUMPAD_ASTERISK,
	DEVICE_KEYCODE_NUMPAD_MINUS,
	DEVICE_KEYCODE_NUMPAD_PLUS,
	DEVICE_KEYCODE_NUMPAD_DOT,
	DEVICE_KEYCODE_NUMPAD_ENTER,

	DEVICE_KEYCODE_F1,
	DEVICE_KEYCODE_F2,
	DEVICE_KEYCODE_F3,
	DEVICE_KEYCODE_F4,
	DEVICE_KEYCODE_F5,
	DEVICE_KEYCODE_F6,
	DEVICE_KEYCODE_F7,
	DEVICE_KEYCODE_F8,
	DEVICE_KEYCODE_F9,
	DEVICE_KEYCODE_F10,
	DEVICE_KEYCODE_F11,
	DEVICE_KEYCODE_F12,

	DEVICE_KEYCODE_COUNT,

	DEVICE_KEYCODE_CODE = 0xff,
	DEVICE_KEYCODE_PRESS = 0x000,
	DEVICE_KEYCODE_RELEASE = 0x100,
} device_keycode_t;

/* mouse */
#define DEVICE_MOUSE_MAX_EVENTS 32

typedef enum device_buttoncode {
	DEVICE_BUTTONCODE_1 = 0,
	DEVICE_BUTTONCODE_2,
	DEVICE_BUTTONCODE_3,

	DEVICE_BUTTONCODE_COUNT,

	DEVICE_BUTTONCODE_LEFT = DEVICE_BUTTONCODE_1,
	DEVICE_BUTTONCODE_MIDDLE = DEVICE_BUTTONCODE_2,
	DEVICE_BUTTONCODE_RIGHT = DEVICE_BUTTONCODE_3,
} device_buttoncode_t;

typedef struct device_mouse_event {
	int x, y; /* x and y motion */
	bool st[DEVICE_BUTTONCODE_COUNT]; /* button state */
} device_mouse_event_t;

typedef struct device_mouse {
	device_t base;
	bool state[DEVICE_BUTTONCODE_COUNT]; /* button state */
	device_mouse_event_t ev[DEVICE_MOUSE_MAX_EVENTS]; /* mouse events */
	int evstart, evend; /* ringbuffer positions */
} device_mouse_t;

/* storage device */
typedef void (*device_storage_read_t)(device_t *, uint32_t, size_t, void *);
typedef void (*device_storage_write_t)(device_t *, uint32_t, size_t, void *);

typedef struct device_storage {
	device_t base;
	bool busy; /* busy flag for multitasking */
	device_storage_read_t read; /* read from storage device */
	device_storage_write_t write; /* write to storage device */
} device_storage_t;

/* device classes */
extern devclass_t devclass_bus; /* device bus class */
extern devclass_t devclass_storage; /* storage class */
extern devclass_t devclass_keyboard; /* keyboard class */
extern devclass_t devclass_mouse; /* mouse class */
extern devclass_t devclass_terminal; /* terminal class (stub) */

/* functions */
extern void device_init(void); /* initialize all devices */
extern void device_update(void); /* update devices */
extern device_t *device_new(devclass_t *cls, const char *desc); /* create new device */
extern void device_print_class(devclass_t *cls); /* print devices in class */
extern void device_print_all(void); /* debug */

extern device_t *device_bus_new(const char *desc); /* create bus device */
extern device_t *device_storage_new(const char *desc); /* create storage device */
extern device_t *device_keyboard_new(const char *desc); /* create keyboard device */
extern device_t *device_mouse_new(const char *desc); /* create mouse device */
extern device_t *device_terminal_new(const char *desc); /* create terminal device (stub) */

extern void device_storage_read(device_t *dev, uint32_t addr, size_t n, void *buf); /* read n blocks from storage device */
extern void device_storage_write(device_t *dev, uint32_t addr, size_t n, void *buf); /* write n blocks to storage device */

extern void device_bus_add(device_t *dev, device_t *child); /* add device to bus */

extern void device_keyboard_putkey(device_t *dev, int key); /* write key to ringbuffer */
extern int device_keyboard_getkey(device_t *dev); /* read key from ringbuffer */
extern int device_keyboard_getkey_block(device_t *dev); /* wait and read key from ringbuffer */

extern void device_mouse_putev(device_t *dev, device_mouse_event_t *ev); /* write event to ringbuffer */
extern device_mouse_event_t *device_mouse_getev(device_t *dev); /* read event from ringbuffer */

#endif /* DEVICE_H */
