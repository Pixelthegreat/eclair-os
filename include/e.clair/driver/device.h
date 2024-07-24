/* simple device manager */
#ifndef DEVICE_H
#define DEVICE_H

#include <e.clair/types.h>

typedef int device_id_t;

/* device types */
typedef enum device_type {
	DEVICE_TYPE_NONE = 0,
	DEVICE_TYPE_INPUT,
	DEVICE_TYPE_STORAGE,

	DEVICE_TYPE_COUNT,
} device_type_t;

/* subtypes */
typedef enum device_subtype {
	DEVICE_SUBTYPE_NONE = 0,
	DEVICE_SUBTYPE_INPUT_PS2,
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

/* input device */
typedef struct device_input {
	device_t base; /* base device info */
} device_input_t;

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

extern void device_storage_read(device_t *dev, uint32_t addr, size_t n, void *buf); /* read n blocks from storage device */
extern void device_storage_write(device_t *dev, uint32_t addr, size_t n, void *buf); /* write n blocks to storage device */

#endif /* DEVICE_H */
