/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_TASK_H
#define ECLAIR_TASK_H

#include <kernel/types.h>
#include <kernel/mm/paging.h>
#include <kernel/vfs/fs.h>

#define TASK_READY 0
#define TASK_RUNNING 1
#define TASK_PAUSED 2
#define TASK_SLEEPING 3
#define TASK_TERMINATED 4
#define TASK_SIGNALED 5
#define TASK_PWAIT 6

#define TASK_NSTATES 7

#define TASK_STACK_START 8
#define TASK_STACK_END 40
#define TASK_STACK_SIZE 0x20000
#define TASK_STACK_ADDR ((void *)0x8000)

#define TASK_ENV_START 40
#define TASK_ENV_ADDR ((void *)0x28000)

#define TASK_PROG_START 0x800
#define TASK_PROG_ADDR ((void *)0x800000)

#define TASK_SIGH_START 7
#define TASK_SIGH_END 8
#define TASK_SIGH_ADDR ((void *)0x7000)

/* have a place to store signal and environment info */
#define TASK_STACK_ADDR_SIGHANDLER ((uint32_t *)0x8000)
#define TASK_STACK_ADDR_SIGEIP ((uint32_t *)0x8004)
#define TASK_STACK_ADDR_ARGV ((uint32_t *)0x8008)
#define TASK_STACK_ADDR_ENVP ((uint32_t *)0x800C)

#define TASK_MINBRKP 0x800000
#define TASK_MAXBRKP 0xB0000000

/* signals */
#define TASK_SIGNONE 0

#define TASK_SIGABRT 1
#define TASK_SIGFPE 2
#define TASK_SIGKILL 3
#define TASK_SIGINT 4
#define TASK_SIGSEGV 5
#define TASK_SIGTERM 6

#define TASK_NSIG 7

typedef void (*task_sig_t)();

/* seek whences */
#define TASK_SEEK_SET 0
#define TASK_SEEK_CUR 1
#define TASK_SEEK_END 2
#define TASK_NWHENCE 3

/* task control block */
#define TASK_MAXFILES 32
#define TASK_MAXMAPPINGS 32

typedef struct task {
	void *esp0; /* kernel stack top */
	void *esp; /* current stack position */
	void *cr3; /* address space */
	page_dir_entry_t *dir; /* page directory */
	struct task *prev; /* previous task */
	struct task *next; /* next task */
	uint32_t state; /* task state */
	uint64_t waketime; /* wake up time for sleeping task */
	uint32_t nticks; /* number of ticks left */
	uint32_t id; /* task id */
	bool ownstack; /* owns kernel stack */
	fs_node_t *res; /* held resource */
	uint32_t sig; /* called signal */
	task_sig_t sigh[TASK_NSIG]; /* signal handlers */
	bool stale; /* a signal changed the task state */
	bool sigdone; /* the signal is finished being handled */
	struct {
		fs_node_t *file; /* vfs node */
		uint32_t flags; /* flags passed from open */
		long pos; /* position in file */
	} files[TASK_MAXFILES]; /* file table */
	struct {
		const char *path; /* path of executable */
		int res; /* result of load process */
	} load; /* info for executable loading */
	uint32_t brkp; /* break point */
	uint32_t entp; /* entry point */
	const char **argv; /* initial argv */
	const char **envp; /* initial envp */
	bool freeargs; /* free argv and envp when done */
	int pwait; /* waiting on process id */
	int wstatus; /* wait status */
	struct {
		bool used; /* free or used */
		page_id_t start; /* start page */
		page_id_t end; /* end page */
	} mappings[TASK_MAXMAPPINGS]; /* special mapped region table */
	int uid; /* user id */
} task_t;

extern task_t *ktask; /* base kernel task */
extern task_t *task_active; /* active task */

extern uint32_t task_nlockpost; /* number of task switch locks */
extern uint32_t task_postponed; /* postponed task switches */

extern uint32_t task_handle_signal_size; /* size of signal handler routine */

/* functions */
extern void task_init_memory(void); /* allocate necessary memory before heap */
extern void task_init(void); /* initialize multitasking */
extern task_t *task_new(void *esp, void *seteip); /* create task */
extern void task_switch(task_t *task); /* switch to next task */
extern void task_schedule(void); /* schedule next task */
extern void task_lockcli(void); /* lock interrupts */
extern void task_unlockcli(void); /* unlock interrupts */
extern uint32_t task_getlockcli(void); /* get number of locks */
extern void task_lockpost(void); /* lock task switches */
extern void task_unlockpost(void); /* unlock task switches */
extern void task_block(uint32_t reason); /* block current task */
extern void task_unblock(task_t *task); /* unblock task */

extern void task_nano_sleep_until(uint64_t waketime); /* sleep in nanoseconds until */
extern void task_nano_sleep(uint64_t ns); /* sleep in nanoseconds */
extern void task_sleep(uint32_t s); /* sleep in seconds */

extern void task_free(void); /* free pages used by current task */
extern void task_terminate(void); /* terminate current task */
extern void task_cleanup(void); /* clean up terminated tasks */
extern void task_acquire(fs_node_t *node); /* acquire resource */
extern void task_release(void); /* release held resource */

extern uint64_t task_get_global_time(void); /* get time for all tasks */
extern void task_entry(void); /* task entry point */

extern void task_raise(uint32_t sig); /* raise signal on current task */
extern void task_signal(task_t *task, uint32_t sig); /* raise signal on other task */
extern void task_handle_signal(void); /* routine to handle signal; do not call directly */
extern task_t *task_get(int id); /* get task from id */
extern void *task_sbrk(intptr_t inc); /* increment or decrement breakpoint */
extern int task_pwait(int pid, uint64_t timeout); /* wait for process status change */
extern int task_mmap(page_id_t area, page_frame_id_t start, page_frame_id_t count); /* make special memory mapping for task */
extern int task_setuser(const char *name, const char *pswd); /* set user for task */

extern int task_fs_open(const char *path, uint32_t flags, uint32_t mask); /* open file */
extern kssize_t task_fs_read(int fd, void *buf, size_t cnt); /* read from file */
extern kssize_t task_fs_write(int fd, void *buf, size_t cnt); /* write to file */
extern koff_t task_fs_seek(int fd, koff_t pos, int whence); /* seek to position */
extern koff_t task_fs_tell(int fd); /* get file position */
extern int task_fs_close(int fd); /* close file */
extern int task_fs_ioctl(int fd, int op, uintptr_t arg); /* send command to io device */

#endif /* ECLAIR_TASK_H */
