/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ec.h>

int main(int argc, const char **argv) {

	int opt;
	int fhelp = 0, fcolor = 0;
	while ((opt = getopt(argc, argv, "hc")) != -1) {
		switch (opt) {
			case 'c':
				fcolor++;
				break;
			case 'h':
				fhelp++;
			default:
				fprintf(stderr, "Usage: %s [-h] [-c]\n", argv[0]);
				return fhelp? 0: 1;
		}
	}

	ec_kinfo_t info;
	ec_kinfo(&info);

	printf("OS name: %s\n"
	       "OS version: %hhu.%hhu.%hhu\n"
	       "Memory usage: %juK/%juK\n",
	       info.name,
	       info.version[0], info.version[1], info.version[2],
	       (info.mem_total - info.mem_free) >> 10, info.mem_total >> 10);

	if (fcolor) {

		fputc('\n', stdout);
		for (int i = 0; i < 8; i++)
			printf("\e[4%dm ", i);
		for (int i = 0; i < 8; i++)
			printf("\e[2;4%dm ", i);
		printf("\e[0m\n");
	}
	return 0;
}
