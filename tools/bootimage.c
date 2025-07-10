/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define BOOTDISK_FILENAME "bootdisk.img"
static FILE *bdfp = NULL;

#define BOOT_FILENAME "build/boot.bin"

/* mbr info */
#define MBR_START 440
#define MBR_SIZE 72

static uint8_t mbr_data[MBR_SIZE];

/* read file contents */
static void *read_file(const char *path, size_t *size) {

	FILE *fp = fopen(path, "rb");
	if (!fp) {

		fprintf(stderr, "Failed to open '%s': %s\n", path, strerror(errno));
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	*size = (size_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* read data */
	void *data = malloc(*size);
	fread(data, 1, *size, fp);
	fclose(fp);
	
	return data;
}

/* run */
static int run(void) {

	bdfp = fopen(BOOTDISK_FILENAME, "rb+");
	if (!bdfp) {

		fprintf(stderr, "Failed to open '%s': %s\n", BOOTDISK_FILENAME, strerror(errno));
		return 1;
	}

	/* read mbr */
	printf("Saving master boot record...\n");
	
	fseek(bdfp, MBR_START, SEEK_SET);
	fread(mbr_data, 1, MBR_SIZE, bdfp);

	/* read bootloader image */
	printf("Writing bootloader...\n");

	size_t datasz;
	void *data = read_file(BOOT_FILENAME, &datasz);
	if (!data) return 1;

	printf("Bootloader size: %lu bytes\n", datasz);

	fseek(bdfp, 0, SEEK_SET);
	fwrite(data, 1, datasz, bdfp);

	/* restore mbr */
	printf("Restoring master boot record...\n");
	
	fseek(bdfp, MBR_START, SEEK_SET);
	fwrite(mbr_data, 1, MBR_SIZE, bdfp);
	return 0;
}

/* clean up resources */
static void cleanup(void) {

	if (bdfp) fclose(bdfp);
}

int main() {

	int code = run();
	cleanup();
	return code;
}
