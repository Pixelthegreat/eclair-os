/* simple device manager */
#ifndef DEVICE_H
#define DEVICE_H

#include <e.clair/types.h>

typedef int device_id_t;

/* device types */
typedef enum device_type: uint32_t {
	DEVICE_TYPE_NONE = 0,
	DEVICE_TYPE_KEYBOARD,

	DEVICE_TYPE_COUNT,
} device_type_t;

/* information for device */
typedef struct device {
	device_id_t id; /* id */
	device_type_t type; /* type */
} device_t;

#endif /* DEVICE_H */
