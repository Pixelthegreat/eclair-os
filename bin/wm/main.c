/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ec.h>
#include <wm/input.h>
#include <wm/screen.h>
#include <wm/window.h>
#include <wm/server.h>

#define STARTUP "/bin/login"

int main() {

	if (input_init() < 0)
		return 1;
	if (screen_init() < 0)
		return 1;
	if (server_host() < 0)
		return 1;

	const char *argv[] = {STARTUP, NULL};
	ec_pexec(STARTUP, argv, NULL);

	while (true) server_update();
}
