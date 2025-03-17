#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/syscall.h>

static idt_isr_t sysh[ECN_COUNT] = {
	NULL,
	sys_exit,
	sys_open,
	sys_read,
	sys_write,
	sys_lseek,
	sys_close,
};

/* handle a generic syscall */
extern void sys_handle(idt_regs_t *regs) {

	uint32_t idx = regs->eax;
	if (idx < ECN_COUNT && sysh[idx]) sysh[idx](regs);
}

/* exit task */
extern void sys_exit(idt_regs_t *regs) {

	task_terminate();
}

/* open file */
extern void sys_open(idt_regs_t *regs) {

	const char *path = (const char *)regs->ebx;
	uint32_t flags = regs->ecx;
	uint32_t mask = regs->edx;

	/* TODO: convert from fcntl flags to vfs flags */

	regs->eax = (uint32_t)task_fs_open(path, flags, mask);
}

/* read from file */
extern void sys_read(idt_regs_t *regs) {

	int fd = (int)regs->ebx;
	void *buf = (void *)regs->ecx;
	size_t cnt = (size_t)regs->edx;

	regs->eax = (uint32_t)task_fs_read(fd, buf, cnt);
}

/* write to file */
extern void sys_write(idt_regs_t *regs) {

	int fd = (int)regs->ebx;
	void *buf = (void *)regs->ecx;
	size_t cnt = (size_t)regs->edx;

	regs->eax = (uint32_t)task_fs_write(fd, buf, cnt);
}

/* seek to position in file */
extern void sys_lseek(idt_regs_t *regs) {

	int fd = (int)regs->ebx;
	koff_t pos = (koff_t)regs->ecx;
	int whence = (int)regs->edx;

	regs->eax = (uint32_t)task_fs_seek(fd, pos, whence);
}

/* close file */
extern void sys_close(idt_regs_t *regs) {

	int fd = (int)regs->ebx;

	regs->eax = (uint32_t)task_fs_close(fd);
}
