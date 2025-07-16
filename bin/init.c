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

	FILE *fp = fopen(linebuf, "r");
	if (!fp) {

		fprintf(stderr, "%s: fopen('%s'): %s\n", argv[0], linebuf, strerror(errno));
		return 1;
	}

	/* load shell process */
	fgets(linebuf, LINEBUFSZ, fp);
	fclose(fp);

	size_t len = strlen(linebuf);
	if (linebuf[len-1] == '\n')
		linebuf[--len] = 0;

	printf("Staring %s...\n", linebuf);

	const char *pargv[] = {linebuf, NULL};
	int pid = ec_pexec(linebuf, pargv, NULL);
	if (pid < 0) {

		fprintf(stderr, "%s: pexec('%s'): %s\n", argv[0], linebuf, strerror(errno));
		return 1;
	}

	/* wait for process to complete execution */
	int status = 0;
	while (!(ECW_ISEXITED(status)))
		ec_pwait(pid, &status, NULL);
	printf("%s: '%s' exited with code %d\n", argv[0], linebuf, ECW_TOEXITCODE(status));

	ec_panic("You're not supposed to be here");
}
