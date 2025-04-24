#ifndef ECLAIR_SYSCALL_H
#define ECLAIR_SYSCALL_H

#include <kernel/types.h>
#include <kernel/idt.h>
#include <ec.h>

/* functions */
extern void sys_handle(idt_regs_t *regs); /* handle a generic syscall */
extern void sys_exit(idt_regs_t *regs); /* exit task */
extern void sys_open(idt_regs_t *regs); /* open file */
extern void sys_read(idt_regs_t *regs); /* read from file */
extern void sys_write(idt_regs_t *regs); /* write to file */
extern void sys_lseek(idt_regs_t *regs); /* seek to position in file */
extern void sys_close(idt_regs_t *regs); /* close file */
extern void sys_stat(idt_regs_t *regs); /* get file info */
extern void sys_fstat(idt_regs_t *regs); /* get open file info */
extern void sys_getpid(idt_regs_t *regs); /* get process id */
extern void sys_kill(idt_regs_t *regs); /* raise a signal on a process */
extern void sys_sbrk(idt_regs_t *regs); /* increase or decrease breakpoint */
extern void sys_gettimeofday(idt_regs_t *regs); /* get epoch time */
extern void sys_timens(idt_regs_t *regs); /* get arbitrary timestamp */
extern void sys_isatty(idt_regs_t *regs); /* check if file is a teletype */
extern void sys_signal(idt_regs_t *regs); /* set a signal handler */

#endif /* ECLAIR_SYSCALL_H */
