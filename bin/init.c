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

#define SH_PATH "/bin/sh"

#define LINEBUFSZ 128
static char linebuf[LINEBUFSZ];

/* main proc */
extern int main(int argc, const char **argv) {

	if (argc != 2) {

		fprintf(stderr, "%s: Invalid arguments\n", argv[0]);
		return 1;
	}
	const char *prof = argv[1];
	snprintf(linebuf, LINEBUFSZ, "/etc/init.%s", prof);

	printf("Starting /bin/sh...\n");

	const char *pargv[] = {SH_PATH, linebuf, NULL};
	int pid = ec_pexec(SH_PATH, pargv, NULL);
	if (pid < 0) {

		fprintf(stderr, "%s: Can't load '" SH_PATH "': %s\n", argv[0], strerror(errno));
		return 1;
	}

	/* wait for process to complete execution */
	int status = 0;
	while (!(ECW_ISEXITED(status)))
		ec_pwait(pid, &status, NULL);
	printf("%s: '" SH_PATH "' exited with code %d\n", argv[0], ECW_TOEXITCODE(status));

	ec_panic("You're not supposed to be here");
}
