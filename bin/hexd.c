/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define READBUFSZ 512
static uint8_t readbuf[READBUFSZ];

#define NCOLUMNS 16

#define IS_PRINTABLE(c) ((c) >= ' ' && (c) <= '~')

/* print hex values */
static void print_hex(size_t nbytes) {

	size_t pos = 0;
	while (pos < nbytes) {

		for (size_t i = 0; i < NCOLUMNS; i++) {

			if (i) printf(" ");

			if (pos+i < nbytes) printf("%hhx%hhx", (readbuf[pos+i] >> 4) & 0xf, readbuf[pos+i] & 0xf);
			else printf("  ");
		}
		printf(" |");
		for (size_t i = 0; i < NCOLUMNS; i++) {

			if (pos+i >= nbytes) break;
			else if (IS_PRINTABLE(readbuf[pos+i])) fputc(readbuf[pos+i], stdout);
			else printf(".");
		}
		printf("|");

		pos += NCOLUMNS;
		printf("\n");
	}
}

int main(int argc, const char **argv) {

	if (argc != 2) {

		fprintf(stderr, "Invalid arguments\nUsage: %s <file>\n", argv[0]);
		return 1;
	}
	const char *path = argv[1];

	FILE *fp = fopen(path, "rb");
	if (!fp) {

		fprintf(stderr, "%s: Can't open '%s': %s\n", argv[0], path, strerror(errno));
		return 1;
	}

	/* read contents */
	size_t nread = READBUFSZ;
	while (nread == READBUFSZ) {

		nread = fread(readbuf, 1, READBUFSZ, fp);
		print_hex(nread);
	}
	fclose(fp);
	return 0;
}
