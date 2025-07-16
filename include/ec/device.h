/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Data for ioctl calls
 */
#ifndef EC_DEVICE_H
#define EC_DEVICE_H

#include <stdint.h>

/*
 * == TTY ==
 */

/*
 * Set or get cursor updates
 *   arg:
 *     0 = Get cursor update state
 *     1 = Flip cursor update state
 */
#define ECIO_TTY_CURSOR 0x02

/*
 * == Framebuffer ==
 */

/*
 * Get framebuffer information
 *   arg: Pointer to fbinfo structure
 */
#define ECIO_FB_GETINFO 0x02

/*
 * Map framebuffer to address
 *   arg: Page aligned address to map
 */
#define ECIO_FB_MAP 0x03

/* framebuffer info */
typedef struct ecio_fbinfo {
	struct {
		uint32_t size; /* mask size */
		uint32_t pos; /* mask position */
	} rmask, gmask, bmask; /* red, green and blue masks */
	uint32_t depth_bits, depth_bytes; /* framebuffer depth */
	uint32_t width, height; /* framebuffer image size */
	uint32_t pitch; /* bytes between horizontal lines */
	uint32_t size; /* total size of framebuffer */
} ecio_fbinfo_t;

/*
 * == Input devices ==
 */

/*
 * Get next event.
 *   For keyboards:
 *     return: Keycode value or zero if there is no event
 *   For mice:
 *     arg: Mouse event (see keycode.h)
 *     return: Zero if successful or one if there is no event
 */
#define ECIO_INP_GETEVENT 0x02

/*
 * Flush input events.
 */
#define ECIO_INP_FLUSH 0x03

#endif /* EC_DEVICE_H */
