/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Simple device manager
 */
#ifndef DEVICE_H
#define DEVICE_H

#include <kernel/types.h>
#include <ec/keycode.h>

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
	int id; /* device id */
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

/* mouse */
#define DEVICE_MOUSE_MAX_EVENTS 32

typedef struct device_mouse_event {
	int x, y; /* x and y motion */
	bool st[ECB_COUNT]; /* button state */
} device_mouse_event_t;

typedef struct device_mouse {
	device_t base;
	bool state[ECB_COUNT]; /* button state */
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

/* audio device */
typedef struct device_audio {
	device_t base;
} device_audio_t;

/* video device */
typedef struct device_video {
	device_t base;
} device_video_t;

/* driver info */
#define DRIVERINFO_PCI 1
#define DRIVERINFO_MBRFS 2

#define DRIVERINFO_NAME_MAX_CHARS 32
typedef struct driverinfo {
	char name[DRIVERINFO_NAME_MAX_CHARS]; /* name of driver */
	int type; /* type of driver */
	void *data; /* associated data */
} __attribute__((aligned(4))) driverinfo_t;

#define DRIVERINFO(pname, ptype, pdata) __attribute__((section(".driverinfo"))) __attribute__((aligned(4))) driverinfo_t driver_##pname = {\
	.name = #pname,\
	.type = ptype,\
	.data = pdata,\
}

extern void _driverinfo_start();
extern void _driverinfo_end();

#define DRIVERINFO_START ((driverinfo_t *)_driverinfo_start)
#define DRIVERINFO_COUNT (((size_t)_driverinfo_end - (size_t)_driverinfo_start) / sizeof(driverinfo_t))

/* device classes */
extern devclass_t devclass_bus; /* device bus class */
extern devclass_t devclass_storage; /* storage class */
extern devclass_t devclass_keyboard; /* keyboard class */
extern devclass_t devclass_mouse; /* mouse class */
extern devclass_t devclass_terminal; /* terminal class (stub) */
extern devclass_t devclass_audio; /* audio controller class */
extern devclass_t devclass_video; /* video adapter class */

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
extern device_t *device_audio_new(const char *desc); /* create audio device */
extern device_t *device_video_new(const char *desc); /* create video device */

extern void device_storage_read(device_t *dev, uint32_t addr, size_t n, void *buf); /* read n blocks from storage device */
extern void device_storage_write(device_t *dev, uint32_t addr, size_t n, void *buf); /* write n blocks to storage device */

extern void device_bus_add(device_t *dev, device_t *child); /* add device to bus */

extern void device_keyboard_putkey(device_t *dev, int key); /* write key to ringbuffer */
extern int device_keyboard_getkey(device_t *dev); /* read key from ringbuffer */
extern int device_keyboard_getkey_block(device_t *dev); /* wait and read key from ringbuffer */
extern void device_keyboard_flushkeys(device_t *dev); /* flush keys */

extern void device_mouse_putev(device_t *dev, device_mouse_event_t *ev); /* write event to ringbuffer */
extern device_mouse_event_t *device_mouse_getev(device_t *dev); /* read event from ringbuffer */
extern void device_mouse_flushevs(device_t *dev); /* flush events */

#endif /* DEVICE_H */
