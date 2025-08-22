/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_MARGIN_H
#define CREPE_MARGIN_H

#include <crepe/core.h>
#include <crepe/widget.h>

/* margin area */
typedef struct crepe_margin {
	crepe_widget_t base;
	size_t left, right; /* left and right margins */
	size_t top, bottom; /* top and bottom margins */
	crepe_widget_t *widget; /* child */
} crepe_margin_t;

#define CREPE_MARGIN(p) ((crepe_margin_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_margin;

/* functions */
extern crepe_widget_t *crepe_margin_new(size_t left, size_t right, size_t top, size_t bottom); /* create margin area */
extern void crepe_margin_child(crepe_margin_t *margin, crepe_widget_t *widget); /* set child */

#endif /* CREPE_MARGIN_H */
