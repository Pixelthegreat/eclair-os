/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/users.h>
#include <kernel/mm/gdt.h>
#include <kernel/driver/pit.h>
#include <kernel/mm/gdt.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/heap.h>
#include <ec.h>
#include <kernel/task.h>

static const char *signames[TASK_NSIG] = {
	"SIGNONE",
	"SIGABRT",
	"SIGFPE",
	"SIGKILL",
	"SIGINT",
	"SIGSEGV",
	"SIGTERM",
};

#define KSTACKSZ 32768 /* process kernel stack size */

#define NTICKS 5

#define FREQ 1193
static const uint64_t FREQ_HZ = PIT_FREQ(FREQ);

task_t *ktask = NULL;
task_t *task_active = NULL;

/* task lists */
struct task_list {
	task_t *first; /* first item in list */
	task_t *last; /* last item in list */
} lists[TASK_NSTATES];

static struct task_list *ready = &lists[TASK_READY];
static struct task_list *paused = &lists[TASK_PAUSED];
static struct task_list *sleeping = &lists[TASK_SLEEPING];
static struct task_list *terminated = &lists[TASK_TERMINATED];
static struct task_list *signaled = &lists[TASK_SIGNALED];
static struct task_list *pwaiting = &lists[TASK_PWAIT];

static uint64_t timens = 0; /* time in nanoseconds */

/* task map */
#define NTASKS 128

static task_t *taskmap[NTASKS]; /* map of tasks associated with id */
static struct {
	page_frame_id_t frame; /* frame */
	page_id_t page; /* page */
} pagedirs[NTASKS]; /* per-task memory space */
static int taskres[NTASKS]; /* task result codes */

/* lock counters */
static uint32_t nlockcli = 0;

uint32_t task_nlockpost = 0;
uint32_t task_postponed = 0; /* postponed task switches */

extern uint32_t kernel_stack_top; /* top of .bss kernel stack */

/* add task to list */
static void task_add_to_list(struct task_list *list, task_t *task) {

	task->prev = list->last;
	task->next = NULL;
	if (!list->first) list->first = task;
	if (list->last) list->last->next = task;
	list->last = task;
}

/* remove task from list */
static void task_remove_from_list(struct task_list *list, task_t *task) {

	if (list->last == task) list->last = task->prev;
	if (list->first == task) list->first = task->next;
	if (task->prev) task->prev->next = task->next;
	if (task->next) task->next->prev = task->prev;
	task->prev = NULL;
	task->next = NULL;
}

/* move from existing list */
static void task_move_to_list(struct task_list *dest, struct task_list *src, task_t *task) {

	task_remove_from_list(src, task);
	task_add_to_list(dest, task);
}

/* cpu exception isr */
static void task_isr(idt_regs_t *regs) {
	
	task_raise(TASK_SIGSEGV);

	/* wait until the signal is handled completely */
	while (!task_active->sigdone) asm volatile("hlt");
}

/* pit irq */
static void task_irq(idt_regs_t *regs) {

	task_lockpost();
	idt_send_eoi();

	timens += 1000000000 / FREQ_HZ;

	/* wake up sleepers */
	task_t *cur = sleeping->first;
	while (cur) {

		task_t *next = cur->next;

		if (timens >= cur->waketime) {

			cur->waketime = 0;
			task_unblock(cur);
		}

		cur = next;
	}

	/* wake up paused tasks */
	cur = paused->first;
	while (cur) {

		task_t *next = cur->next;

		if (cur->res && !fs_isheld(cur->res)) {

			cur->res->held = true;
			task_unblock(cur);
		}

		cur = next;
	}

	/* wake up signaled tasks */
	cur = signaled->first;
	while (cur) {

		task_t *next = cur->next;

		task_unblock(cur);
		cur = next;
	}

	/* wake up tasks waiting on other tasks */
	cur = pwaiting->first;
	while (cur) {

		task_t *next = cur->next;

		if (cur->waketime && timens >= cur->waketime) {

			cur->waketime = 0;
			cur->wstatus = ECW_TIMEOUT;
			task_unblock(cur);
		}
		else {

			task_t *wtask = task_get(cur->pwait);
			if (!wtask || wtask->state == TASK_TERMINATED) {

				int res = (cur->pwait < 0 || cur->pwait >= NTASKS)? 0: taskres[cur->pwait];
				cur->wstatus = res | ECW_EXITED;
				task_unblock(cur);
			}
		}
		cur = next;
	}

	/* end of time slice */
	if (!task_active->nticks || !--task_active->nticks)
		task_schedule();

	task_unlockpost();

	/* task was signaled */
	if (task_active->sig) {

		task_lockcli();

		uint32_t sig = task_active->sig;
		task_active->sig = 0;

		task_sig_t sigh = task_active->sigh[sig];
		if (!sigh) {

			kprintf(LOG_WARNING, "[task] Signal %d received (%s, task %d); Aborting...", (int)sig, signames[sig], (int)task_active->id);

			task_unlockcli();
			task_terminate();
		}

		*TASK_STACK_ADDR_SIGHANDLER = (uint32_t)sigh;
		*TASK_STACK_ADDR_SIGEIP = regs->eip;

		regs->eip = (uint32_t)TASK_SIGH_ADDR;

		task_active->sigdone = true;
		task_unlockcli();
	}
}

/* allocate necessary memory before heap */
extern void task_init_memory(void) {

	for (int i = 0; i < NTASKS; i++) {

		pagedirs[i].frame = page_frame_alloc();
		pagedirs[i].page = page_breakp++;
		page_map(pagedirs[i].page, pagedirs[i].frame);
	}
}

/* initialize multitasking */
extern void task_init(void) {

	ktask = task_new(&kernel_stack_top, NULL);
	ktask->cr3 = page_get_directory();
	ktask->dir = page_dir_wrap;
	ktask->state = TASK_RUNNING;
	ktask->nticks = NTICKS;
	task_remove_from_list(ready, ktask); /* remove from ready list */

	task_active = ktask;

	/* setup isrs */
	idt_set_isr_callback(IDT_ISR_GPFAULT, task_isr);
	idt_set_isr_callback(IDT_ISR_PGFAULT, task_isr);

	/* setup pit */
	idt_disable_irq_eoi(PIT_IRQ);
	pit_set_callback(task_irq);

	pit_set_mode(PIT_COMMAND(PIT_CHANNEL0, PIT_ACCESS_HILO, PIT_MODE_SQUARE));

	pit_set_channel(PIT_CHANNEL0, FREQ & 0xff);
	pit_set_channel(PIT_CHANNEL0, (FREQ >> 8) & 0xff);
}

/* create task */
extern task_t *task_new(void *esp, void *seteip) {

	task_lockcli();

	/* get task id */
	uint32_t id = 0;
	for (; id < NTASKS && taskmap[id]; id++);
	if (id >= NTASKS) {

		task_unlockcli();
		return NULL;
	}

	/* create task */
	task_t *task = (task_t *)kmalloc(sizeof(task_t));

	task->ownstack = false;
	if (!esp) {
		
		esp = kmalloc(KSTACKSZ);
		memset(esp, 0, KSTACKSZ);
		esp += KSTACKSZ;
		task->ownstack = true;
	}

	task->esp0 = esp;
	task->esp = esp;
	task->cr3 = NULL;
	task->dir = NULL;
	task->state = TASK_READY;
	task->waketime = 0;
	task->nticks = NTICKS;
	task->id = id;
	task->res = NULL;
	task->sig = 0;
	for (uint32_t i = 0; i < TASK_NSIG; i++)
		task->sigh[i] = NULL;
	task->stale = false;
	task->sigdone = false;
	for (uint32_t i = 0; i < TASK_MAXFILES; i++) {
		task->files[i].file = NULL;
		task->files[i].flags = 0;
		task->files[i].pos = 0;
	}
	task->load.path = NULL;
	task->load.res = 0;
	task->brkp = (uint32_t)TASK_PROG_ADDR;
	task->argv = NULL;
	task->envp = NULL;
	task->freeargs = false;
	task->pwait = 0;
	task->wstatus = 0;
	for (uint32_t i = 0; i < TASK_MAXMAPPINGS; i++) {
		task->mappings[i].used = false;
		task->mappings[i].start = 0;
		task->mappings[i].end = 0;
	}
	task->uid = 0;

	task_add_to_list(ready, task);
	taskmap[id] = task;

	/* clone page directory */
	if (id) {
		
		task->cr3 = page_clone_directory(pagedirs[id].frame, pagedirs[id].page);
		task->dir = (page_dir_entry_t *)PAGE_ADDR(pagedirs[id].page);
	}

	/* add eip to stack */
	if (seteip != NULL) {

		task->esp -= 4;
		*(uint32_t *)task->esp = (uint32_t)seteip;
		task->esp -= 16;
	}

	task_unlockcli();
	return task;
}

/* schedule next task */
extern void task_schedule(void) {

	if (task_nlockpost) {

		task_postponed = 1;
		return;
	}

	/* only schedule if task is available */
	if (ready->first != NULL) {

		task_t *next = ready->first;
		task_remove_from_list(ready, next);
		next->state = TASK_RUNNING;
		next->nticks = NTICKS;

		if (task_active->state == TASK_RUNNING) {

			task_add_to_list(ready, task_active);
			task_active->state = TASK_READY;
			task_active->nticks = NTICKS;
		}

		task_switch(next);
	}
}

/* lock interrupts */
extern void task_lockcli(void) {

	asm volatile("cli");
	nlockcli++;
}

/* unlock interrupts */
extern void task_unlockcli(void) {

	if (!(--nlockcli))
		asm volatile("sti");
}

/* get number of locks */
extern uint32_t task_getlockcli(void) {

	return nlockcli;
}

/* lock task switches */
extern void task_lockpost(void) {

	asm volatile("cli");
	nlockcli++;
	task_nlockpost++;
}

/* unlock task switches */
extern void task_unlockpost(void) {

	if (!(--task_nlockpost) && task_postponed) {

		task_postponed = 0;
		task_schedule();
	}

	if (!(--nlockcli))
		asm volatile("sti");
}

/* block current task */
extern void task_block(uint32_t reason) {

	task_lockcli();

	task_active->state = reason;
	task_add_to_list(&lists[reason], task_active);
	task_schedule();

	task_unlockcli();
}

/* unblock task */
extern void task_unblock(task_t *task) {

	if (task == task_active || task->state == TASK_READY)
		return;

	task_lockcli();

	task_t *first = ready->first;
	
	task_remove_from_list(&lists[task->state], task);
	task->state = TASK_READY;
	task_add_to_list(ready, task);

	if (!first) task_schedule();

	task_unlockcli();
}

/* sleep in nanoseconds until */
extern void task_nano_sleep_until(uint64_t waketime) {

	if (waketime < timens)
		return;

	task_active->waketime = waketime;
	task_block(TASK_SLEEPING);
}

/* sleep in nanoseconds */
extern void task_nano_sleep(uint64_t ns) {

	task_nano_sleep_until(timens + ns);
}

/* sleep in seconds */
extern void task_sleep(uint32_t s) {

	task_nano_sleep((uint64_t)s * 1000000000);
}

/* free pages used by current task */
extern void task_free(void) {

	task_lockcli();

	/* unmap mappings */
	for (uint32_t i = 0; i < TASK_MAXMAPPINGS; i++) {
		if (task_active->mappings[i].used) {
			for (uint32_t j = task_active->mappings[i].start; j < task_active->mappings[i].end; j++)
				page_unmap(j);
		}
	}

	/* free frames */
	uint32_t brkp = ALIGN(task_active->brkp, 4096) >> 12;
	for (uint32_t i = 0; i < brkp; i++) {

		page_frame_id_t f = page_get_frame(i);
		if (f) page_frame_free(f);
	}

	/* fun fact: the lack of this block of code was the cause of a memory leak */
	brkp = ALIGN(brkp, 1024) >> 10;
	for (uint32_t i = 0; i < brkp; i++) {

		page_frame_id_t f = page_get_table_frame(i);
		if (f) page_frame_free(f);
	}

	/* close files */
	for (int i = 0; i < TASK_MAXFILES; i++) {

		if (task_active->files[i].file)
			(void)task_fs_close(i);
	}

	task_unlockcli();
}

/* terminate current task */
extern void task_terminate(void) {

	taskres[task_active->id] = task_active->load.res & 0xff;
	task_free();
	task_block(TASK_TERMINATED);
}

/* clean up terminated tasks */
extern void task_cleanup(void) {

	if (terminated->first) {

		task_lockcli();
		while (terminated->first) {

			task_t *task = terminated->first;
			task_remove_from_list(terminated, task);

			/* free stack and other resources */
			taskmap[task->id] = NULL;

			if (task->ownstack) kfree(task->esp0-KSTACKSZ);
			kfree(task);
		}
		task_unlockcli();
	}
}

/* acquire resource */
extern void task_acquire(fs_node_t *node) {

	task_lockcli();
	task_active->res = node;

	/* other task held */
	if (fs_isheld(node)) {

		task_unlockcli();
		task_block(TASK_PAUSED);
	}

	/* available */
	else {

		node->held = true;
		task_unlockcli();
	}
}

/* release held resource */
extern void task_release(void) {

	if (!task_active->res)
		return;

	task_lockcli();
	task_active->res->held = false;
	task_active->res = NULL;
	task_unlockcli();
}

/* get time for all tasks */
extern uint64_t task_get_global_time(void) {

	return timens;
}

/* generic task entry point */
extern void task_entry(void) {

	task_unlockcli();

	/* map user stack */
	for (uint32_t i = TASK_STACK_START; i < TASK_STACK_END; i++)
		page_map_flags(i, page_frame_alloc(), PAGE_FLAG_US);

	void *stack = TASK_STACK_ADDR;
	memset(stack, 0, TASK_STACK_SIZE);
	stack += TASK_STACK_SIZE;

	gdt_tss.esp0 = (uint32_t)task_active->esp0;

	/* copy task_handle_signal function to be able to run it in userspace */
	for (uint32_t i = TASK_SIGH_START; i < TASK_SIGH_END; i++)
		page_map_flags(i, page_frame_alloc(), PAGE_FLAG_US);

	memcpy(TASK_SIGH_ADDR, task_handle_signal, task_handle_signal_size);

	/* copy arguments and environment */
	if (task_active->argv && task_active->envp) {

		size_t size = 0;
		size_t argc = 0;
		size_t envc = 0;

		size_t i = 0;
		while (task_active->argv[i]) {
			argc++;
			size += sizeof(const char *);
			size += strlen(task_active->argv[i]) + 1;
			i++;
		}
		size += sizeof(const char *); /* account for NULL at end */
		i = 0;
		while (task_active->envp[i]) {
			envc++;
			size += sizeof(const char *);
			size += strlen(task_active->envp[i]) + 1;
			i++;
		}
		size += sizeof(const char *);

		/* allocate memory for environment */
		size_t npages = ALIGN(size, 0x1000) >> 12;
		for (uint32_t j = TASK_ENV_START; j < TASK_ENV_START+npages; j++)
			page_map_flags(j, page_frame_alloc(), PAGE_FLAG_US);

		/* copy data */
		const char **ptr = (const char **)TASK_ENV_ADDR;
		char *str = (char *)ptr + (argc + envc + 2) * sizeof(const char *);

		i = 0;
		*TASK_STACK_ADDR_ARGV = (uint32_t)ptr;

		for (size_t j = 0; j < argc; j++) {

			ptr[i++] = str;
			size_t len = strlen(task_active->argv[j]);

			memcpy(str, task_active->argv[j], len);
			str[len++] = 0;
			str += len;
		}
		ptr[i++] = NULL;
		*TASK_STACK_ADDR_ENVP = (uint32_t)&ptr[i];

		for (size_t j = 0; j < envc; j++) {

			ptr[i++] = str;
			size_t len = strlen(task_active->envp[j]);

			memcpy(str, task_active->envp[j], len);
			str[len++] = 0;
			str += len;
		}
		ptr[i++] = NULL;

		if (task_active->freeargs) {

			task_lockcli();
			kfree((void *)task_active->load.path);
			kfree(task_active->argv);
			kfree(task_active->envp);
			task_unlockcli();
		}
	}

	/* go to user mode */
	asm volatile(
		"cli\n"
		"mov %0, %%esp\n"
		"mov $35, %%ax\n"
		"mov %%ax, %%ds\n"
		"mov %%ax, %%es\n"
		"mov %%ax, %%fs\n"
		"mov %%ax, %%gs\n"
		"mov %%esp, %%eax\n"
		"sti\n"
		"push $35\n" /* data segment */
		"push %%eax\n" /* stack pointer */
		"pushf\n" /* eflags */
		"push $27\n" /* code segment */
		"push %1\n" /* entry point address */
		"iret\n"
		: : "r"(stack), "r"(task_active->entp));
}

/* raise signal on current task */
extern void task_raise(uint32_t sig) {

	task_active->sig = sig;
	task_active->sigdone = false;
	task_block(TASK_SIGNALED);
}

/* raise signal on other task */
extern void task_signal(task_t *task, uint32_t sig) {

	if (task == task_active) {

		task_raise(sig);
		return;
	}

	task_lockcli();

	task_remove_from_list(&lists[task->state], task);
	task->state = TASK_SIGNALED;
	task_add_to_list(signaled, task);

	task->stale = true;
	task->sigdone = false;
	task->sig = sig;

	task_unlockcli();
}

/* get task from id */
extern task_t *task_get(int id) {

	if (id < 0 || id >= NTASKS)
		return NULL;
	return taskmap[id];
}

/* increment or decrement breakpoint */
extern void *task_sbrk(intptr_t inc) {

	task_lockcli();

	void *ptr = (void *)task_active->brkp;

	uint32_t brkp = task_active->brkp + (uint32_t)inc;
	if (brkp < TASK_MINBRKP || brkp >= TASK_MAXBRKP) {

		task_unlockcli();
		return NULL;
	}
	
	uint32_t obrkp = task_active->brkp;

	/* allocate pages */
	if (brkp > obrkp) {

		uint32_t start = ALIGN(obrkp, 0x1000) >> 12;
		uint32_t end = ALIGN(brkp, 0x1000) >> 12;

		for (uint32_t i = start; i < end; i++) {
			if (!page_is_mapped(i))
				page_map_flags(i, page_frame_alloc(), PAGE_FLAG_US);
		}
	}

	/* free pages */
	else {

		uint32_t start = ALIGN(brkp, 0x1000) >> 12;
		uint32_t end = ALIGN(obrkp, 0x1000) >> 12;

		for (uint32_t i = start; i < end; i++) {

			page_frame_id_t fr = page_get_frame(i);
			if (fr) {
				page_frame_free(fr);
				page_unmap(i);
			}
		}
	}

	task_active->brkp = brkp;

	task_unlockcli();
	return ptr;
}

/* wait for process status change */
extern int task_pwait(int pid, uint64_t timeout) {

	task_lockcli();
	task_t *task = task_get(pid);
	if (!task || task->state == TASK_TERMINATED) {

		task_unlockcli();
		int res = (pid < 0 || pid >= NTASKS)? 0: taskres[pid];
		return res | ECW_EXITED;
	}
	task_active->pwait = pid;
	task_active->wstatus = 0;
	task_active->waketime = timeout + timens;
	task_unlockcli();
	
	task_active->stale = false;
	task_block(TASK_PWAIT);
	if (task_active->stale) return -EINTR;

	return task_active->wstatus;
}

/* make special memory mapping for task */
extern int task_mmap(page_id_t area, page_frame_id_t start, page_frame_id_t count) {

	task_lockcli();
	int mapping = -1;

	/* determine mapping to use */
	for (int i = 0; i < TASK_MAXMAPPINGS; i++) {
		if (task_active->mappings[i].used) {

			/* check if area overlaps */
			if ((area >= task_active->mappings[i].start && area < task_active->mappings[i].end) ||
			    (area+count >= task_active->mappings[i].start && area+count < task_active->mappings[i].end)) {

				task_unlockcli();
				return -EINVAL;
			}
		}
		else if (mapping < 0) mapping = i;
	}
	if (mapping < 0) {

		task_unlockcli();
		return -ENOBUFS;
	}

	/* map memory */
	task_active->mappings[mapping].used = true;
	task_active->mappings[mapping].start = area;
	task_active->mappings[mapping].end = area+count;

	for (page_frame_id_t i = 0; i < count; i++) {

		page_frame_id_t active = page_get_frame(area+i);
		if (active) {

			page_frame_free(active);
			page_map_flags(area+i, start+i, PAGE_FLAG_US);
		}
	}

	task_unlockcli();
	return 0;
}

/* set user for task */
extern int task_setuser(const char *name, const char *pswd) {

	user_t *user = user_get(0);
	while (user) {

		if (!strcmp(user->name, name))
			break;
		user = user->next;
	}
	if (!user) return -EINVAL;

	/* compare password hashes */
	if (user->pswd != strhash(pswd))
		return -EACCES;

	task_active->uid = user->uid;
	return 0;
}

/* compare user id and permissions */
static int check_perm(int uid, int gid, uint32_t flags, uint32_t mask) {

	uint32_t r = 0, w = 0;

	user_t *user = user_get(uid);
	user_t *muser = user_get(task_active->uid);
	if (!user || !muser) return -1;

	/* owner, group or other */
	if (uid == task_active->uid) {

		r = mask & 0400;
		w = mask & 0200;
	}
	else if (gid == muser->gid) {

		r = mask & 040;
		w = mask & 020;
	}
	else {

		r = mask & 04;
		w = mask & 02;
	}

	/* file flags */
	if ((flags & FS_READ) && !r) return -1;
	if ((flags & FS_WRITE) && !w) return -1;

	return 0;
}

/* open file */
extern int task_fs_open(const char *path, uint32_t flags, uint32_t mask) {

	/* find usable file descriptor */
	int fd = 0;
	for (; fd < TASK_MAXFILES && task_active->files[fd].file; fd++);
	if (fd >= TASK_MAXFILES) return -EMFILE;

	/* locate file */
	task_lockcli();
	
	bool create = false;
	const char *fname = NULL;
	fs_node_t *node = fs_resolve_full(path, &create, &fname);

	task_unlockcli();
	if ((create && !(flags & FS_CREATE)) || !node)
		return -ENOENT;

	task_active->stale = false;
	task_acquire(node);
	if (task_active->stale) return -EAGAIN;

	/* create file */
	if (create && (flags & FS_CREATE)) {

		fs_node_t *next = fs_create(node, fname, FS_FILE, mask);
		task_release();

		if (!next) return -EACCES;
		node = next;

		node->uid = (uint32_t)task_active->uid;
		node->gid = node->uid; /* FIXME: This is not correct */

		task_active->stale = false;
		task_acquire(node);
		if (task_active->stale) return -EAGAIN;
	}

	/* check file permissions */
	if (check_perm((int)node->uid, (int)node->gid, flags, node->mask) < 0) {

		task_release();
		return -EACCES;
	}

	/* determine compatible open flags */
	uint32_t oflags = 0;
	if (node->refcnt) {

		bool invflags = false;
		if (!FS_ISRW(node->oflags)) {

			if ((oflags & FS_READ) != (node->oflags & FS_READ)) invflags = true;
			if ((oflags & FS_WRITE) != (node->oflags & FS_WRITE)) invflags = true;
		}
		if ((oflags & FS_TRUNCATE) != (node->oflags & FS_TRUNCATE)) invflags = true;

		if (invflags) {

			task_release();
			return -EINVAL;
		}
		oflags = node->oflags;
	}
	else oflags = flags & 0xff; /* mask off FS_CREATE and related */

	/* open file */
	fs_open(node, oflags);

	task_active->files[fd].file = node;
	task_active->files[fd].flags = flags;
	task_active->files[fd].pos = 0;

	task_release();
	return fd;
}

/* read from file */
extern kssize_t task_fs_read(int fd, void *buf, size_t cnt) {

	if (fd < 0 || fd >= TASK_MAXFILES || !task_active->files[fd].file)
		return -EBADF;
	if (!buf) return -EINVAL;
	fs_node_t *node = task_active->files[fd].file;

	if (!(task_active->files[fd].flags & FS_READ))
		return -EBADF;

	task_active->stale = false;
	task_acquire(node);
	if (task_active->stale) return -EAGAIN;

	kssize_t nread = fs_read(node, (uint32_t)task_active->files[fd].pos, cnt, (uint8_t *)buf);
	task_release();

	if (nread >= 0) task_active->files[fd].pos += nread;

	return nread;
}

/* write to file */
extern kssize_t task_fs_write(int fd, void *buf, size_t cnt) {

	if (fd < 0 || fd >= TASK_MAXFILES || !task_active->files[fd].file)
		return -EBADF;
	if (!buf) return -EINVAL;
	fs_node_t *node = task_active->files[fd].file;

	if (!(task_active->files[fd].flags & FS_WRITE))
		return -EBADF;

	task_active->stale = false;
	task_acquire(node);
	if (task_active->stale) return -EAGAIN;

	kssize_t nwrite = fs_write(node, (uint32_t)task_active->files[fd].pos, cnt, (uint8_t *)buf);
	task_release();

	if (nwrite >= 0) task_active->files[fd].pos += nwrite;

	return nwrite;
}

/* seek to position */
extern koff_t task_fs_seek(int fd, koff_t pos, int whence) {

	if (fd < 0 || fd >= TASK_MAXFILES || !task_active->files[fd].file)
		return -EBADF;
	if (whence < 0 || whence >= TASK_NWHENCE)
		return -EINVAL;

	if (whence == TASK_SEEK_SET) task_active->files[fd].pos = pos;
	else if (whence == TASK_SEEK_CUR) task_active->files[fd].pos += pos;
	else if (whence == TASK_SEEK_END) task_active->files[fd].pos = task_active->files[fd].file->len - pos;

	task_active->files[fd].pos = CLAMP(task_active->files[fd].pos, 0, task_active->files[fd].file->len);
	return task_active->files[fd].pos;
}

/* get file position */
extern koff_t task_fs_tell(int fd) {

	if (fd < 0 || fd >= TASK_MAXFILES || !task_active->files[fd].file)
		return -EBADF;
	return task_active->files[fd].pos;
}

/* close file */
extern int task_fs_close(int fd) {

	if (fd < 0 || fd >= TASK_MAXFILES || !task_active->files[fd].file)
		return -EBADF;
	fs_node_t *node = task_active->files[fd].file;

	task_active->stale = false;
	if (!nlockcli) task_acquire(node);
	if (task_active->stale) return -EAGAIN;

	fs_close(node);
	if (!nlockcli) task_release();

	task_active->files[fd].file = NULL;
	return 0;
}

/* send command to io device */
extern int task_fs_ioctl(int fd, int op, uintptr_t arg) {

	if (fd < 0 || fd >= TASK_MAXFILES || !task_active->files[fd].file)
		return -EBADF;
	fs_node_t *node = task_active->files[fd].file;

	task_active->stale = false;
	task_acquire(node);
	if (task_active->stale) return -EAGAIN;

	int res = fs_ioctl(node, op, arg);
	task_release();

	return res;
}
