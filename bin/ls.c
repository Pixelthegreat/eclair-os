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

		printf("%s\n", dent.name);
	}
	if (res < 0) {

		fprintf(stderr, "%s: readdir('%s'): %s\n", argv[0], path, strerror(errno));
		return 1;
	}
	return 0;
}
