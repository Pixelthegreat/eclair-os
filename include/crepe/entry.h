/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_ENTRY_H
#define CREPE_ENTRY_H

#include <crepe/core.h>
#include <crepe/widget.h>

/* entry */
#define CREPE_ENTRY_MAX_CHARS 128

typedef struct crepe_entry {
	crepe_widget_t base;
	bool active; /* active/selected */
	bool shift; /* shift pressed */
	char text[CREPE_ENTRY_MAX_CHARS]; /* entry text */
	size_t pos; /* cursor position */
} crepe_entry_t;

#define CREPE_ENTRY(p) ((crepe_entry_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_entry;

/* functions */
extern crepe_widget_t *crepe_entry_new(void); /* create entry */

#endif /* CREPE_ENTRY_H */
