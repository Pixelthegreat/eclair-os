/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_PANIC_H
#define ECLAIR_PANIC_H

#include <kernel/idt.h>

/* panic codes */
typedef enum panic_code {
	PANIC_CODE_NONE = 0,
	PANIC_CODE_FAULT,

	PANIC_CODE_COUNT,
} panic_code_t;
extern int kerrno;

/* log levels */
typedef enum log_level {
	LOG_INFO = 0,
	LOG_WARNING,
	LOG_FATAL,

	LOG_COUNT,
} log_level_t;

/* functions */
extern void kpanic(int code, const char *msg, idt_regs_t *regs); /* print panic message */
extern void kprintf(log_level_t level, const char *fmt, ...); /* print log message */

#endif /* ECLAIR_PANIC_H */
