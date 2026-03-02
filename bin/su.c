/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ec.h>

#define SH_PATH "/bin/sh"
#define RC_PATH "/etc/ecrc.sh"

#define BUFSZ 32
static char namebuf[BUFSZ];
static char pswdbuf[BUFSZ];

/* execute shell */
static int exec_shell(const char *progname, const char **argv) {

	const char *sh_argv[] = {SH_PATH, NULL};
	if (!argv) argv = sh_argv;

	int pid = ec_pexec(argv[0], argv, NULL);
	if (pid < 0) {

		fprintf(stderr, "%s: Can't execute '%s': %s\n",
			progname, argv[0], strerror(errno));
		return 1;
	}

	int status = 0;
	while (!ECW_ISEXITED(status))
		ec_pwait(pid, &status, NULL);

	return status & ECW_EXITCODE;
}

/* main */
int main(int argc, const char **argv) {

	int opt, fhelp = 0, fretry = 0;
	int usrind = argc, endind = argc;

	while ((opt = getopt(argc, argv, "hr-")) != -1) {
		switch (opt) {
			case 'r':
				fretry++;
				break;
			case '-':
				usrind = optind-1;
				if (usrind > 1 && *argv[usrind-1] != '-')
					usrind--;
				endind = optind;
				goto next;
			case 'h':
				fhelp++;
			default:
				fprintf(stderr, "Usage: %s [-h] [-r] [user] [--] [command]\n",
					argv[0]);
				return fhelp? 0: 1;
		}
	}
	if (usrind == argc) {
		
		usrind = optind;
		endind = usrind+1;
	}
next:
	const char *user = usrind < argc? argv[usrind]: NULL;
	const char **cmd_argv = endind < argc? argv+endind: NULL;

	/* loop */
	if (!user || !strcmp(user, "--")) {

		int result = 0;
		do {

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

			if (ec_setuser(namebuf, pswdbuf) < 0) {

				fprintf(stderr, "%s: Can't set user '%s': %s\n",
					argv[0], namebuf, strerror(errno));
				result = 1;
			}
			else exec_shell(argv[0], cmd_argv);

		} while (fretry);
	}

	/* enter just password */
	else {
		printf("Password: ");
		fflush(stdout);

		fgets(pswdbuf, BUFSZ, stdin);
		size_t len = strlen(pswdbuf);
		if (pswdbuf[len-1] == '\n')
			pswdbuf[--len] = 0;

		if (ec_setuser(user, pswdbuf) < 0) {

			fprintf(stderr, "%s: Can't set user '%s': %s\n",
				argv[0], user, strerror(errno));
			return 1;
		}
		return exec_shell(argv[0], cmd_argv);
	}
	return 0;
}
