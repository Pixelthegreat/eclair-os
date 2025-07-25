/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdlib.h>
#include <ec.h>

const char **__libc_argv = NULL;
const char **__libc_envp = NULL;
const char ***__libc_base = (const char ***)0x8008;

const char **environ = NULL;

extern void __libc_main(int argc, const char **argv);

static void sigsegv() {

	ec_write(1, "Segmentation fault\n", 19);
	abort();
}

extern void _start() {

	__libc_argv = *__libc_base;
	__libc_envp = *(__libc_base+1);
	environ = __libc_envp;

	int argc = 0;
	while (__libc_argv[argc]) argc++;

	ec_signal(5, sigsegv);

	/* open stdin, stdout and stderr */
	ec_open(getenv("EC_STDIN"), ECF_READ, 0);
	ec_open(getenv("EC_STDOUT"), ECF_WRITE, 0);
	ec_open(getenv("EC_STDERR"), ECF_WRITE, 0);

	ec_chdir(getenv("PWD"));

	__libc_main(argc, __libc_argv);
}
