/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_BOX_H
#define CREPE_BOX_H

#include <crepe/core.h>
#include <crepe/widget.h>

/* box orientations */
typedef enum crepe_box_orientation {
	CREPE_BOX_ORIENTATION_HORIZONTAL = 0,
	CREPE_BOX_ORIENTATION_VERTICAL,
} crepe_box_orientation_t;

/* box */
#define CREPE_BOX_MAX_ITEMS 32

typedef struct crepe_box {
	crepe_widget_t base;
	crepe_box_orientation_t orientation; /* orientation of items */
	crepe_widget_t *items[CREPE_BOX_MAX_ITEMS]; /* box items */
	size_t nitems; /* number of items */
} crepe_box_t;

#define CREPE_BOX(p) ((crepe_box_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_box;

/* functions */
extern crepe_widget_t *crepe_box_new(crepe_box_orientation_t orientation); /* create box */
extern void crepe_box_item(crepe_box_t *box, crepe_widget_t *item); /* add item to box */

#endif /* CREPE_BOX_H */
