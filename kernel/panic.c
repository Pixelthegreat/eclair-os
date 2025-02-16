#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/idt.h>
#include <kernel/panic.h>

static const char *codedesc[PANIC_CODE_COUNT] = {
	NULL,
	"Page fault", /* page fault */
};
static const char *logdesc[LOG_COUNT] = {
	"\e[32m Info  \e[39m",
	"\e[33mWarning\e[39m",
	"\e[31m Fatal \e[39m",
};

#define CODEDESC(c, msg) (((c) <= 0 || (c) >= PANIC_CODE_COUNT)? (msg): codedesc[(c)])

/* print panic message */
extern void kpanic(int code, const char *msg, idt_regs_t *regs) {

	tty_printf("\x1b[41;2;31mKernel panic! %s\n", CODEDESC(code, msg));

	/* print registers */
	if (regs) {

		tty_printf("Internal registers:\nEIP=0x%x, EFLAGS=0x%x\n\n", regs->eip, regs->eflags);
		tty_printf("General purpose registers:\nEAX=0x%x, EBX=0x%x, ECX=0x%x, EDX=0x%x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
		tty_printf("ESP=0x%x, EBP=0x%x, ESI=0x%x, EDI=0x%x\n", regs->esp, regs->ebp, regs->esi, regs->edi);
	}

	/* disable interrupts and loop */
	asm("cli");
	while (1) asm("hlt");
}

/* print log message */
extern void kprintf(log_level_t level, const char *fmt, ...) {

	tty_printf("[%s] ", logdesc[level]);
	
	va_list args;
	va_start(args, fmt);
	tty_vprintf(fmt, args);
	va_end(args);

	tty_printnl();
}
