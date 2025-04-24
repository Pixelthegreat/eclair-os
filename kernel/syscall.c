#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/task.h>
#include <kernel/driver/rtc.h>
#include <kernel/syscall.h>

static idt_isr_t sysh[ECN_COUNT] = {
	[ECN_EXIT] = sys_exit,
	[ECN_OPEN] = sys_open,
	[ECN_READ] = sys_read,
	[ECN_WRITE] = sys_write,
	[ECN_LSEEK] = sys_lseek,
	[ECN_CLOSE] = sys_close,
	[ECN_STAT] = sys_stat,
	[ECN_FSTAT] = sys_fstat,
	[ECN_GETPID] = sys_getpid,
	[ECN_KILL] = sys_kill,
	[ECN_SBRK] = sys_sbrk,
	[ECN_GETTIMEOFDAY] = sys_gettimeofday,
	[ECN_TIMENS] = sys_timens,
	[ECN_ISATTY] = sys_isatty,
	[ECN_SIGNAL] = sys_signal,
};

#define RETURN_ERROR(c) ({\
		regs->eax = (uint32_t)(c);\
		return;\
	})

/* get file information */
static void nstat(fs_node_t *node, ec_stat_t *st) {

	memset(st, 0, sizeof(ec_stat_t));

	st->ino = (int)node->inode;
	st->mode = (ec_mode_t)node->mask;
	st->nlink = 1;
	st->uid = (int)node->uid;
	st->gid = (int)node->gid;
	st->size = (long)node->len;

	fs_stat(node, st);
}

/* handle a generic syscall */
extern void sys_handle(idt_regs_t *regs) {

	uint32_t idx = regs->eax;
	if (idx < ECN_COUNT && sysh[idx]) sysh[idx](regs);
	else regs->eax = (uint32_t)-1;
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

/* get file info */
extern void sys_stat(idt_regs_t *regs) {

	const char *path = (const char *)regs->ebx;
	ec_stat_t *st = (ec_stat_t *)regs->ecx;

	fs_node_t *node = fs_resolve(path);
	if (!node) RETURN_ERROR(-1);

	nstat(node, st);
	regs->eax = 0;
}

/* get open file info */
extern void sys_fstat(idt_regs_t *regs) {

	int fd = (int)regs->ebx;
	ec_stat_t *st = (ec_stat_t *)regs->ecx;

	if (fd < 0 || fd >= TASK_MAXFILES)
		RETURN_ERROR(-1);

	fs_node_t *node = task_active->files[fd].file;
	if (!node) RETURN_ERROR(-1);

	nstat(node, st);
	regs->eax = 0;
}

/* get process id */
extern void sys_getpid(idt_regs_t *regs) {

	regs->eax = (uint32_t)task_active->id;
}

/* raise a signal on a process */
extern void sys_kill(idt_regs_t *regs) {

	int pid = (int)regs->ebx;
	int sig = (int)regs->ecx;

	task_t *task = task_get(pid);
	if (!task) RETURN_ERROR(-1);

	task_signal(task, (uint32_t)sig);

	regs->eax = 0;
}

/* increase or decrease breakpoint */
extern void sys_sbrk(idt_regs_t *regs) {

	intptr_t inc = (intptr_t)regs->ebx;

	void *brkp = task_sbrk(inc);
	if (!brkp) RETURN_ERROR(0);

	regs->eax = (uint32_t)brkp;
}

/* get epoch time */
extern void sys_gettimeofday(idt_regs_t *regs) {

	ec_timeval_t *tv = (ec_timeval_t *)regs->ebx;

	rtc_cmos_regs_t cmos;
	rtc_get_registers(&cmos);

	tv->sec = rtc_get_time(&cmos);
	tv->nsec = 0;

	regs->eax = 0;
}

/* get arbitrary timestamp */
extern void sys_timens(idt_regs_t *regs) {

	ec_timeval_t *tv = (ec_timeval_t *)regs->ebx;

	uint64_t timens = task_get_global_time();

	tv->sec = timens / 1000000000;
	tv->nsec = timens % 1000000000;

	regs->eax = 0;
}

/* check if file is a teletype */
extern void sys_isatty(idt_regs_t *regs) {

	int fd = (int)regs->ebx;
	if (fd < 0 || fd >= TASK_MAXFILES)
		RETURN_ERROR(-1);

	fs_node_t *node = task_active->files[fd].file;
	if (!node) RETURN_ERROR(-1);

	regs->eax = (uint32_t)fs_isatty(node);
}

/* set a signal handler */
extern void sys_signal(idt_regs_t *regs) {

	int sig = (int)regs->ebx;
	task_sig_t sigh = (task_sig_t)regs->ecx;

	if (sig < 0 || sig >= TASK_NSIG)
		RETURN_ERROR(-1);

	task_active->sigh[sig] = sigh;
	regs->eax = 0;
}
