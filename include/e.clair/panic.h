#ifndef ECLAIR_PANIC_H
#define ECLAIR_PANIC_H

#include <e.clair/idt.h>

/* panic codes */
enum {
	PANIC_CODE_NONE = 0,
	PANIC_CODE_FAULT,

	PANIC_CODE_COUNT,
};
extern int kerrno;

/* functions */
extern void kpanic(int code, const char *msg, idt_regs_t *regs); /* print panic message */

#endif /* ECLAIR_PANIC_H */
