/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *s_optstring = NULL;
const char *optarg = NULL;
int optind = 0;
static const char *arg = NULL;
static int index = 0;

extern int getopt(int argc, const char *const *argv, const char *optstring) {

	/* reset state */
	if (s_optstring != optstring) {

		s_optstring = optstring;
		optarg = NULL;
		optind = 1;
		index = 1;
		arg = NULL;
	}

	/* parse arguments */
	while (index < argc || (arg && *arg)) {

		if (!arg || !*arg) {

			arg = argv[index++];
			if (*arg != '-') {

				arg = NULL;
				continue;
			}
		}

		for (arg++; *arg; arg++) {

			const char *opt = strchr(optstring, *arg);
			if (!opt || *opt == ':') {

				fprintf(stderr, "%s: Unrecognized option '-%c'\n",
					argv[0], *arg);
				return '?';
			}
			optind = index;

			if (*(opt+1) != ':') return *arg;

			/* option value immediately after */
			if (*(arg+1)) {

				optarg = arg+1;
				arg = NULL;
			}

			/* option value in next argument */
			else if (!argv[index]) {

				fprintf(stderr, "%s: Expected argument after option '-%c'\n",
					argv[0], *arg);
				return '?';
			}
			else optarg = argv[index++];
			optind = index;
			return *opt;
		}
	}
	return -1;
}
