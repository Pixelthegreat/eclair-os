/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_IMAGE_H
#define CREPE_IMAGE_H

#include <crepe/core.h>
#include <crepe/widget.h>

/* image */
typedef struct crepe_image {
	crepe_widget_t base;
	uint32_t image; /* image handle */
	size_t width, height; /* image size */
} crepe_image_t;

#define CREPE_IMAGE(p) ((crepe_image_t *)(p))

extern crepe_widget_ops_t crepe_widget_ops_image;

/* functions */
extern crepe_widget_t *crepe_image_new_from_file(const char *path); /* create image from file */

#endif /* CREPE_IMAGE_H */
