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
#define RC_PATH "/etc/ecrc.sh"

#define BUFSZ 32
static char namebuf[BUFSZ];
static char pswdbuf[BUFSZ];

/* execute shell */
static int exec_shell(const char *progname) {

	const char *argv[] = {SH_PATH, NULL, NULL};
	if (ec_getpid() == 2) argv[1] = RC_PATH;

	int pid = ec_pexec(SH_PATH, argv, NULL);
	if (pid < 0) {

		fprintf(stderr, "%s: Can't execute '%s': %s\n", progname, SH_PATH, strerror(errno));
		return 1;
	}

	int status = 0;
	while (!ECW_ISEXITED(status))
		ec_pwait(pid, &status, NULL);

	return status & ECW_EXITCODE;
}

/* main */
int main(int argc, const char **argv) {

	if (argc != 2) {
		while (1) {

			printf("Username: ");
			fflush(stdout);

			fgets(namebuf, BUFSZ, stdin);
			size_t len = strlen(namebuf);
			if (namebuf[len-1] == '\n')
				namebuf[--len] = 0;

			printf("Password: ");
			fflush(stdout);

			fgets(pswdbuf, BUFSZ, stdin);
			len = strlen(pswdbuf);
			if (pswdbuf[len-1] == '\n')
				pswdbuf[--len] = 0;

			if (ec_setuser(namebuf, pswdbuf) < 0)
				fprintf(stderr, "%s: Can't set user '%s': %s\n", argv[0], namebuf, strerror(errno));
			else exec_shell(argv[0]);
		}
	}

	/* enter just password */
	else {

		const char *name = argv[1];

		printf("Password: ");
		fflush(stdout);

		fgets(pswdbuf, BUFSZ, stdin);
		size_t len = strlen(pswdbuf);
		if (pswdbuf[len-1] == '\n')
			pswdbuf[--len] = 0;

		if (ec_setuser(name, pswdbuf) < 0) {

			fprintf(stderr, "%s: Can't set user '%s': %s\n", argv[0], name, strerror(errno));
			return 1;
		}
		return exec_shell(argv[0]);
	}
	return 0;
}
