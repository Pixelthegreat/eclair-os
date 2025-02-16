/* simple device manager */
#ifndef DEVICE_H
#define DEVICE_H

#include <kernel/types.h>

typedef int device_id_t;

/* device types */
typedef enum device_type {
	DEVICE_TYPE_NONE = 0,
	DEVICE_TYPE_CHAR,
	DEVICE_TYPE_STORAGE,
	DEVICE_TYPE_TIMER,

	DEVICE_TYPE_COUNT,
} device_type_t;

/* subtypes */
typedef enum device_subtype {
	DEVICE_SUBTYPE_NONE = 0,
	DEVICE_SUBTYPE_CHAR_PS2,
	DEVICE_SUBTYPE_CHAR_UART,
	DEVICE_SUBTYPE_STORAGE_ATA,
	DEVICE_SUBTYPE_STORAGE_FLOPPY,

	DEVICE_SUBTYPE_COUNT,
} device_subtype_t;

/* information for device */
#define DEVICE_DESC_MAX_CHARS 32

typedef struct device {
	device_id_t id; /* id */
	device_type_t type; /* type */
	device_subtype_t subtype; /* subtype */
	char desc[DEVICE_DESC_MAX_CHARS]; /* device description */
	uint32_t impl; /* implementation specific value */
} device_t;

/* character device */
typedef void (*device_char_flush_t)(device_t *);

#define DEVICE_CHAR_BUFSZ 32

typedef struct device_char {
	device_t base; /* base device info */
	uint32_t s_ibuf, e_ibuf; /* start and end of input buffer */
	uint32_t s_obuf, e_obuf; /* start and end of output buffer */
	uint32_t ibuf[DEVICE_CHAR_BUFSZ]; /* input ring buffer */
	uint32_t obuf[DEVICE_CHAR_BUFSZ]; /* output ring buffer */
	device_char_flush_t flush; /* flush data of output buffer */
} device_char_t;

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
	DEVICE_KEYCODE_9,

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
} device_keycode_t;

/* keycode masks */
#define DEVICE_KEYCODE_CODE 0xff
#define DEVICE_KEYCODE_PRESS 0x000
#define DEVICE_KEYCODE_RELEASE 0x100

/* storage device */
typedef void (*device_storage_read_t)(device_t *, uint32_t, size_t, void *);
typedef void (*device_storage_write_t)(device_t *, uint32_t, size_t, void *);

typedef struct device_storage {
	device_t base; /* base device info */
	bool busy; /* busy flag for multitasking */
	device_storage_read_t read; /* read from storage device */
	device_storage_write_t write; /* write to storage device */
} device_storage_t;

/* functions */
extern void device_init(void); /* initialize all devices */
extern device_t *device_new(device_type_t type, device_subtype_t subtype, const char *name, size_t sz); /* create new device */
extern void device_print_all(void); /* debug */
extern device_t *device_find(device_type_t type, device_subtype_t subtype, int n); /* find nth device of type and subtype */
extern void device_translate_biosdev(uint32_t dev, device_subtype_t *subtp, int *n); /* translate a bios device number */
extern device_t *device_find_root(void); /* search for root device */

extern uint32_t device_char_read(device_t *dev, bool block); /* read next int from device */
extern void device_char_write(device_t *dev, uint32_t val, bool flush); /* write next int to device */

extern void device_storage_read(device_t *dev, uint32_t addr, size_t n, void *buf); /* read n blocks from storage device */
extern void device_storage_write(device_t *dev, uint32_t addr, size_t n, void *buf); /* write n blocks to storage device */

#endif /* DEVICE_H */
