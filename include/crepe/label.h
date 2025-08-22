/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_LABEL_H
#define CREPE_LABEL_H

#include <crepe/core.h>
#include <crepe/draw.h>
#include <crepe/widget.h>

/* label */
typedef struct crepe_label {
	crepe_widget_t base;
	crepe_text_style_t style; /* text style */
	const char *text; /* label text */
} crepe_label_t;

#define CREPE_LABEL(p) ((crepe_label_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_label;

/* functions */
extern crepe_widget_t *crepe_label_new(crepe_text_style_t style, const char *text); /* create label */

#endif /* CREPE_LABEL_H */
