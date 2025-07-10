/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Common definitions for simple image formats and interface for image loading
 */
#ifndef EC_IMAGE_H
#define EC_IMAGE_H

#include <stdint.h>
#include <stdio.h>

/* robin header */
typedef enum ec_rbn_mode {
	EC_RBN_MODE_R5G5B5 = 0,
	EC_RBN_MODE_R5G6B5,
	EC_RBN_MODE_R8G8B8,
	EC_RBN_MODE_R8G8B8A8,

	EC_RBN_MODE_COUNT,
} ec_rbn_mode_t;

typedef enum ec_rbn_compression {
	EC_RBN_COMPRESSION_NONE = 0,
	EC_RBN_COMPRESSION_RLE,

	EC_RBN_COMPRESSION_COUNT,
} ec_rbn_compression_t;

#define EC_RBN_SIGNATURE "\0RBN"

typedef struct ec_rbn_header {
	char signature[4]; /* signature */
	uint32_t version; /* version = 1 */
	uint32_t width, height; /* image size */
	uint32_t mode; /* data mode/format */
	uint32_t compression; /* compression mode/format */
} __attribute__((packed)) ec_rbn_header_t;

/* image formats */
typedef enum ec_image_format {
	EC_IMAGE_FORMAT_RBN = 0,

	EC_IMAGE_FORMAT_COUNT,
} ec_image_format_t;

/* image data formats */
typedef enum ec_image_data {
	EC_IMAGE_DATA_RGB8 = 0,
	EC_IMAGE_DATA_RGBA8,

	EC_IMAGE_DATA_COUNT,
} ec_image_data_t;

/* image operations */
struct ec_image;

typedef struct ec_image_ops {
	int (*read_header)(struct ec_image *); /* read header info */
	int (*read_color)(struct ec_image *, uint8_t *, ec_image_data_t); /* read color from image */
} ec_image_ops_t;

/* generic image info */
typedef struct ec_image {
	FILE *fp; /* open file */
	size_t width, height; /* image size */
	size_t position; /* internal color position */
	ec_image_ops_t *ops; /* interal image operations */
	uint32_t impl[4]; /* interal loader implementation details */
} ec_image_t;

#define EC_IMAGE_INIT ((ec_image_t){.fp = NULL})

/*
 * Open an image to read.
 *
 * 'format' is the file format, not the data format.
 *
 * Returns zero if successful, negative on error.
 */
extern int ec_image_open(ec_image_t *image, const char *path, ec_image_format_t format);

/*
 * Read one or more colors from an image file.
 *
 * 'format' is the data format to read (rgb8, rgba8, ...).
 *
 * Returns zero if successful, negative on error.
 */
extern int ec_image_read_colors(ec_image_t *image, uint8_t *buffer, size_t count, ec_image_data_t format);

/*
 * Close an image.
 *
 * Returns zero if successful, negative on error.
 */
extern int ec_image_close(ec_image_t *image);

#endif /* EC_IMAGE_H */
