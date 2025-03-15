#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/mm/gdt.h>
#include <kernel/driver/pit.h>
#include <kernel/mm/gdt.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/heap.h>
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

#define KSTACKSZ 16384 /* process kernel stack size */

#define NTICKS 10

#define FREQ 1193
static const uint64_t FREQ_HZ = PIT_FREQ(FREQ);

task_t *ktask = NULL;
task_t *task_active = NULL;

/* task lists */
struct task_list {
	task_t *first; /* first item in list */
	task_t *last; /* last item in list */
} lists[TASK_NSTATES];

static struct task_list *ready = NULL;
static struct task_list *paused = NULL;
static struct task_list *sleeping = NULL;
static struct task_list *terminated = NULL;
static struct task_list *signaled = NULL;

static uint64_t timens = 0; /* time in nanoseconds */

/* task map */
#define NTASKS 128

static task_t *taskmap[NTASKS]; /* map of tasks associated with id */
static struct {
	page_frame_id_t frame; /* frame */
	page_id_t page; /* page */
} pagedirs[NTASKS]; /* per-task memory space */

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

		if (timens >= cur->waketime)
			task_unblock(cur);

		cur = next;
	}

	/* wake up paused tasks */
	cur = paused->first;
	while (cur) {

		task_t *next = cur->next;

		if (cur->res && !cur->res->held) {

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

	/* end of time slice */
	if (task_active->nticks && !--task_active->nticks)
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

	ready = &lists[TASK_READY];
	paused = &lists[TASK_PAUSED];
	sleeping = &lists[TASK_SLEEPING];
	terminated = &lists[TASK_TERMINATED];
	signaled = &lists[TASK_SIGNALED];

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

	task_lockpost();

	if (waketime < timens) {

		task_unlockpost();
		return;
	}

	task_active->waketime = waketime;
	task_block(TASK_SLEEPING);

	task_unlockpost();
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

	for (uint32_t i = 0; i < TASK_STACK_END; i++) {

		page_frame_id_t f = page_get_frame(i);
		if (f) page_frame_free(f);
	}

	task_unlockcli();
}

/* terminate current task */
extern void task_terminate(void) {

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
	if (node->held) {

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

	task_lockcli();
	if (!task_active->res) {

		task_unlockcli();
		return;
	}

	task_active->res->held = false;
	task_active->res = NULL;
	task_unlockcli();
}

/* get time for all tasks */
extern uint64_t task_get_global_time(void) {

	return timens;
}

/* task entry point */
static void task_test(void) {

	int counter = 0;
	while (counter++ < 33554432);

	asm volatile("hlt");
	while (1);
}

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
	for (uint32_t i = TASK_SIGH_START; i< TASK_SIGH_END; i++)
		page_map_flags(i, page_frame_alloc(), PAGE_FLAG_US);

	memcpy(TASK_SIGH_ADDR, task_handle_signal, task_handle_signal_size);

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
		"push $task_test\n"
		"iret\n"
		: : "r"(stack));
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
