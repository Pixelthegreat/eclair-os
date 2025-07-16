/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec.h>

/* print file mode */
static void print_mode(uint32_t mask) {

	char mstr[10] = "---------";

	if (mask & 0400) mstr[0] = 'r';
	if (mask & 0200) mstr[1] = 'w';
	if (mask & 0100) mstr[2] = 'x';
	if (mask & 040) mstr[3] = 'r';
	if (mask & 020) mstr[4] = 'w';
	if (mask & 010) mstr[5] = 'x';
	if (mask & 04) mstr[6] = 'r';
	if (mask & 02) mstr[7] = 'w';
	if (mask & 01) mstr[8] = 'x';

	printf("%s", mstr);
}

int main(int argc, const char **argv) {

	const char *path = getenv("PWD");
	if (argc >= 2) path = argv[1];

	if (!path) return 1;

	/* read directory entries */
	ec_dirent_t dent;
	int res;
	const char *pstr = path;
	while (!(res = ec_readdir(pstr, &dent))) {

		pstr = NULL;

		if (dent.flags & ECS_DIR) printf(" <DIR> ");
		else printf("       ");

		print_mode(dent.mask);

		printf(" %s\n", dent.name);
	}
	if (res < 0) {

		fprintf(stderr, "%s: Can't ls '%s': %s\n", argv[0], path, strerror(errno));
		return 1;
	}
	return 0;
}
