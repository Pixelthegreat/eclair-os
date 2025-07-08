/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec/image.h>

#define SETERRNO(code) ({\
		errno = code;\
		return -1;\
	})

/* robin mode depths */
static size_t rbn_depths[EC_RBN_MODE_COUNT] = {
	[EC_RBN_MODE_R5G5B5] = 16,
	[EC_RBN_MODE_R5G6B5] = 16,
	[EC_RBN_MODE_R8G8B8] = 24,
	[EC_RBN_MODE_R8G8B8A8] = 32,
};

/* read robin header */
static int rbn_read_header(ec_image_t *image) {

	ec_rbn_header_t header;
	if (fread(&header, 1, sizeof(header), image->fp) != sizeof(header) ||
	    !!memcmp(header.signature, EC_RBN_SIGNATURE, 4) ||
	    header.version != 1)
		SETERRNO(-EBADF);

	image->width = (size_t)header.width;
	image->height = (size_t)header.height;
	image->impl[0] = header.mode;
	image->impl[1] = header.compression;
	image->impl[2] = 0; /* repeat count */
	image->impl[3] = 0; /* repeat color */

	return 0;
}

/* read color value from robin image to uint32_t */
static int rbn_next_color(ec_image_t *image, uint32_t *color) {

	uint8_t data[4];
	size_t nbytes = rbn_depths[image->impl[0]] / 8;
	if (fread(data, 1, nbytes, image->fp) != nbytes)
		return -1;

	/* convert color */
	uint32_t v16;
	switch (image->impl[0]) {
		case EC_RBN_MODE_R5G5B5:
			v16 = (uint32_t)data[0] | ((uint32_t)data[1] << 8);
			*color = ((v16 & 0x1f) << 3) | (((v16 >> 5) & 0x1f) << 11) | (((v16 >> 10) & 0x1f) << 19);
			break;
		case EC_RBN_MODE_R5G6B5:
			v16 = (uint32_t)data[0] | ((uint32_t)data[1] << 8);
			*color = ((v16 & 0x1f) << 3) | (((v16 >> 5) & 0x3f) << 10) | (((v16 >> 11) & 0x1f) << 19);
			break;
		case EC_RBN_MODE_R8G8B8:
			*color = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | 0xff000000;
			break;
		case EC_RBN_MODE_R8G8B8A8:
			*color = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
			break;
	}
	return 0;
}

/* read color from robin image */
static int rbn_read_color(ec_image_t *image, uint8_t *buffer, ec_image_data_t format) {

	uint32_t mode = image->impl[0];
	uint32_t compression = image->impl[1];
	uint32_t color = 0;

	/* run length compression */
	if (compression == EC_RBN_COMPRESSION_RLE) {
		if (image->impl[2]) {

			color = image->impl[3];
			image->impl[2]--;
		}
		else {

			if (rbn_next_color(image, &color) < 0)
				return -1;
			uint8_t repeat = 0;
			if (fread(&repeat, 1, 1, image->fp) != 1)
				return -1;

			image->impl[2] = (uint32_t)repeat-1;
			image->impl[3] = color;
		}
	}

	/* not compressed */
	else if (rbn_next_color(image, &color) < 0)
		return -1;

	/* convert to expected format */
	switch (format) {
		case EC_IMAGE_DATA_RGB8:
			buffer[0] = (uint8_t)(color & 0xff);
			buffer[1] = (uint8_t)((color >> 8) & 0xff);
			buffer[2] = (uint8_t)((color >> 16) & 0xff);
			break;
		case EC_IMAGE_DATA_RGBA8:
			buffer[0] = (uint8_t)(color & 0xff);
			buffer[1] = (uint8_t)((color >> 8) & 0xff);
			buffer[2] = (uint8_t)((color >> 16) & 0xff);
			buffer[3] = (uint8_t)((color >> 24) & 0xff);
			break;
	}
	return 0;
}

/* format operations */
static ec_image_ops_t ops[EC_IMAGE_FORMAT_COUNT] = {
	[EC_IMAGE_FORMAT_RBN] = {
		.read_header = rbn_read_header,
		.read_color = rbn_read_color,
	},
};

/* open image to read */
extern int ec_image_open(ec_image_t *image, const char *path, ec_image_format_t format) {

	if (image->fp || !path || format < 0 || format >= EC_IMAGE_FORMAT_COUNT)
		SETERRNO(-EINVAL);

	/* open file and read image header */
	image->fp = fopen(path, "rb");
	if (!image->fp) return -1;

	image->ops = &ops[format];
	if (image->ops->read_header(image) < 0) {

		fclose(image->fp);
		image->fp = NULL;
		return -1;
	}
	image->position = 0;

	return 0;
}

/* read colors from file */
extern int ec_image_read_colors(ec_image_t *image, uint8_t *buffer, size_t count, ec_image_data_t format) {

	if (!image || !image->fp || !buffer || format < 0 || format >= EC_IMAGE_DATA_COUNT)
		SETERRNO(-EINVAL);

	for (size_t i = 0; i < count && i+image->position < image->width * image->height; i++) {

		if (image->ops->read_color(image, buffer + i * (format+3), format) < 0)
			return -1;
	}
	return 0;
}

/* close image */
extern int ec_image_close(ec_image_t *image) {

	if (!image || !image->fp) SETERRNO(-EINVAL);

	fclose(image->fp);
	image->fp = NULL;

	return 0;
}
