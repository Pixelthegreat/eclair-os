/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>

int main() {

	printf("\e[2J\e[3J\e[H");
	fflush(stdout);
	return 0;
}
