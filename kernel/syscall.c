#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/task.h>
#include <kernel/elf.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/rtc.h>
#include <errno.h>
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
	[ECN_PANIC] = sys_panic,
	[ECN_PEXEC] = sys_pexec,
	[ECN_PWAIT] = sys_pwait,
	[ECN_SLEEPNS] = sys_sleepns,
	[ECN_READDIR] = sys_readdir,
	[ECN_IOCTL] = sys_ioctl,
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
	st->flags = node->flags;
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
	else regs->eax = (uint32_t)-ENOSYS;
}

/* exit task */
extern void sys_exit(idt_regs_t *regs) {

	int code = (int)regs->ebx;

	task_active->load.res = code;
	task_terminate();
}

/* open file */
extern void sys_open(idt_regs_t *regs) {

	const char *path = (const char *)regs->ebx;
	uint32_t flags = regs->ecx;
	uint32_t mask = regs->edx;

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

	if (!path || !st)
		RETURN_ERROR(-EINVAL);

	fs_node_t *node = fs_resolve(path);
	if (!node) RETURN_ERROR(-ENOENT);

	nstat(node, st);
	regs->eax = 0;
}

/* get open file info */
extern void sys_fstat(idt_regs_t *regs) {

	int fd = (int)regs->ebx;
	ec_stat_t *st = (ec_stat_t *)regs->ecx;

	if (fd < 0 || fd >= TASK_MAXFILES)
		RETURN_ERROR(-EBADF);

	fs_node_t *node = task_active->files[fd].file;
	if (!node) RETURN_ERROR(-EBADF);

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
	if (!task) RETURN_ERROR(-ESRCH);

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
		RETURN_ERROR(-EBADF);

	fs_node_t *node = task_active->files[fd].file;
	if (!node) RETURN_ERROR(-EBADF);

	regs->eax = (uint32_t)fs_isatty(node);
}

/* set a signal handler */
extern void sys_signal(idt_regs_t *regs) {

	int sig = (int)regs->ebx;
	task_sig_t sigh = (task_sig_t)regs->ecx;

	if (sig < 0 || sig >= TASK_NSIG)
		RETURN_ERROR(-EINVAL);

	task_active->sigh[sig] = sigh;
	regs->eax = 0;
}

/* cause a kernel panic */
extern void sys_panic(idt_regs_t *regs) {

	const char *msg = (const char *)regs->ebx;

	if (!msg) RETURN_ERROR(-EINVAL);
	if (task_active->id != 1)
		RETURN_ERROR(-EPERM);

	kpanic(PANIC_CODE_NONE, msg, regs);
	regs->eax = 0;
}

/* execute a process */
extern void sys_pexec(idt_regs_t *regs) {

	const char *path = (const char *)regs->ebx;
	const char **argv = (const char **)regs->ecx;
	const char **envp = (const char **)regs->edx;

	if (!path || !argv)
		RETURN_ERROR(-EINVAL);
	if (!envp) envp = *((const char ***)TASK_STACK_ADDR_ENVP);

	/* reconstruct path */
	size_t pathlen = strlen(path) + 1;
	char *npath = (char *)kmalloc(pathlen);
	memcpy(npath, path, pathlen);

	/* reconstruct argv */
	size_t size = 0;
	size_t pos = 0;
	size_t i = 0;
	while (argv[i]) {

		size_t len = strlen(argv[i]) + 1;

		size += sizeof(const char *);
		size += len;
		i++;
	}
	size += sizeof(const char *); i++;
	pos = i * sizeof(const char *);

	task_lockcli();
	const char **nargv = kmalloc(size);
	task_unlockcli();

	for (size_t j = 0; j < i; j++) {

		char *arg = (char *)nargv + pos;
		nargv[j] = argv[j]? arg: NULL;
		if (argv[j]) {

			size_t len = strlen(argv[j]) + 1;
			memcpy(arg, argv[j], len);
			pos += len;
		}
	}

	/* reconstruct envp */
	size = 0;
	pos = 0;
	i = 0;
	while (envp[i]) {

		size_t len = strlen(envp[i]) + 1;

		size += sizeof(const char *);
		size += len;
		i++;
	}
	size += sizeof(const char *); i++;
	pos = i * sizeof(const char *);

	task_lockcli();
	const char **nenvp = kmalloc(size);
	task_unlockcli();

	for (size_t j = 0; j < i; j++) {

		char *arg = (char *)nenvp + pos;
		nenvp[j] = envp[j]? arg: NULL;
		if (envp[j]) {

			size_t len = strlen(envp[j]) + 1;
			memcpy(arg, envp[j], len);
			pos += len;
		}
	}

	/* run process */
	int pid = elf_load_task(npath, nargv, nenvp, true);
	if (pid < 0) {

		task_lockcli();
		kfree(npath);
		kfree(nargv);
		kfree(nenvp);
		task_unlockcli();

		RETURN_ERROR(pid);
	}
	regs->eax = (uint32_t)pid;
}

/* wait for a process to change status */
extern void sys_pwait(idt_regs_t *regs) {

	int pid = (int)regs->ebx;
	int *wstatus = (int *)regs->ecx;
	ec_timeval_t *timeout = (ec_timeval_t *)regs->edx;

	if (!wstatus) RETURN_ERROR(-EINVAL);

	uint64_t vtimeout = timeout? (timeout->sec * 1000000000) + timeout->nsec: 100000000000000;
	int res = task_pwait(pid, vtimeout);

	if (res < 0) RETURN_ERROR(res);
	*wstatus = res;
	regs->eax = 0;
}

/* sleep for fixed amount of time */
extern void sys_sleepns(idt_regs_t *regs) {

	ec_timeval_t *tv = (ec_timeval_t *)regs->ebx;
	if (!tv) RETURN_ERROR(-EINVAL);

	uint64_t ns = (tv->sec * 1000000000) + tv->nsec;

	task_active->stale = false;
	task_nano_sleep(ns);
	if (task_active->stale)
		RETURN_ERROR(-EINTR);

	regs->eax = 0;
}

/* read directory entries */
extern void sys_readdir(idt_regs_t *regs) {

	const char *path = (const char *)regs->ebx;
	ec_dirent_t *dent = (ec_dirent_t *)regs->ecx;

	if (!dent) RETURN_ERROR(-EINVAL);

	fs_dirent_t *fdent = NULL;

	/* find and acquire resource */
	if (path) {

		memset(dent, 0, sizeof(ec_dirent_t));

		fs_node_t *node = fs_resolve(path);
		if (!node) RETURN_ERROR(-ENOENT);

		while (node->ptr) node = node->ptr;

		if (!(node->flags & FS_DIRECTORY))
			RETURN_ERROR(-ENOTDIR);

		/* update directory */
		if (!node->first) {

			task_active->stale = false;
			task_acquire(node);
			if (task_active->stale)
				RETURN_ERROR(-EINTR);

			fdent = fs_readdir(node, 0);
			task_release();
		}
		else fdent = node->first;
		dent->_data = (void *)fdent;
	}

	/* get next entry */
	else {

		fdent = (fs_dirent_t *)dent->_data;
		fdent = fdent->next;
		dent->_data = (void *)fdent;
	}

	/* fill dirent */
	if (!fdent) RETURN_ERROR(1);

	strncpy(dent->name, fdent->name, ECD_NAMESZ);
	dent->flags = fdent->node? fdent->node->flags: 0;

	regs->eax = 0;
}

/* send command to io device */
extern void sys_ioctl(idt_regs_t *regs) {

	int fd = (int)regs->ebx;
	int op = (int)regs->ecx;
	uintptr_t arg = (uintptr_t)regs->edx;

	regs->eax = (uint32_t)task_fs_ioctl(fd, op, arg);
}
