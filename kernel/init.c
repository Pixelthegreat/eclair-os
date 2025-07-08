/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/elf.h>
#include <kernel/boot.h>
#include <kernel/init.h>

static const char *prog = "/bin/init";
static const char *argv[3];

/* load init program */
extern void init_load(void) {

	const char *prof = NULL;
	boot_cmdline_t *cmdline = boot_get_cmdline();
	if (cmdline) prof = cmdline->init_profile;

	/* construct arguments and environment */
	argv[0] = prog;
	argv[1] = prof;
	argv[2] = NULL;

	static const char *env[] = {
		"EC_STDIN=/dev/tty0",
		"EC_STDOUT=/dev/tty0",
		"EC_STDERR=/dev/tty0",
		"PWD=/",
		NULL,
	};

	if (elf_load_task(prog, argv, env, false) < 0)
		kpanic(PANIC_CODE_NONE, "Failed to load init process", NULL);
	kprintf(LOG_INFO, "[init] Loaded init process '%s' with profile '%s'", prog, prof);
}
