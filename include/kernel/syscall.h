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
extern void sys_isatty(idt_regs_t *regs); /* check if file is a teletype */

#endif /* ECLAIR_SYSCALL_H */
