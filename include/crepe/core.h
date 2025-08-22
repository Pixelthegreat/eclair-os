/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CREPE_CORE_H
#define CREPE_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CREPE_MIN(x, y) ((x) < (y)? (x): (y))
#define CREPE_MAX(x, y) ((x) > (y)? (x): (y))

/* result */
typedef enum crepe_result {
	CREPE_RESULT_SUCCESS = 0,
	CREPE_RESULT_FAILURE,

	CREPE_RESULT_COUNT,
} crepe_result_t;

#endif /* CREPE_CORE_H */
