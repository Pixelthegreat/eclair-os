#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/driver/pit.h>
#include <kernel/mm/heap.h>
#include <kernel/mm/paging.h>
#include <kernel/task.h>

#define FREQ 1193

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

static uint64_t timens = 0; /* time in nanoseconds */

/* lock counters */
static uint32_t nlockcli = 0;

uint32_t task_nlockpost = 0;
uint32_t task_postponed = 0; /* postponed task switches */

extern uint32_t kernel_stack_top;

/* add task to list */
static void task_add_to_list(struct task_list *list, task_t *task) {

	task->prev = list->last;
	task->next = list->first;
	if (!list->first) list->first = task;
	if (list->last) task->next = list->last;
}

/* remove task from list */
static void task_remove_from_list(struct task_list *list, task_t *task) {

	if (list->last == task) list->last = task->prev;
	if (list->first == task) list->first = task->next;
	task->prev = NULL;
	task->next = NULL;
}

/* move from existing list */
static void task_move_to_list(struct task_list *dest, struct task_list *src, task_t *task) {

	task_remove_from_list(src, task);
	task_add_to_list(dest, task);
}

/* pit irq */
static void task_irq(idt_regs_t *regs) {

	task_lockpost();

	timens += 1000000000 / PIT_FREQ(FREQ);

	/* wake up sleepers */
	task_t *cur = sleeping->first;
	while (cur) {

		task_t *next = cur->next;

		if (timens >= cur->waketime)
			task_unblock(cur);

		cur = next;
	}

	task_unlockpost();
}

/* initialize multitasking */
extern void task_init(void) {

	ready = &lists[TASK_READY];
	paused = &lists[TASK_PAUSED];
	sleeping = &lists[TASK_SLEEPING];

	ktask = task_new(&kernel_stack_top, NULL);
	ktask->cr3 = page_get_directory();
	ktask->state = TASK_RUNNING;
	task_remove_from_list(ready, ktask); /* remove from ready list */

	task_active = ktask;

	/* setup pit */
	pit_set_callback(task_irq);

	pit_set_mode(PIT_COMMAND(PIT_CHANNEL0, PIT_ACCESS_HILO, PIT_MODE_SQUARE));

	pit_set_channel(PIT_CHANNEL0, FREQ & 0xff);
	pit_set_channel(PIT_CHANNEL0, (FREQ >> 8) & 0xff);
}

/* create task */
extern task_t *task_new(void *esp, void *seteip) {

	task_t *task = (task_t *)kmalloc(sizeof(task_t));
	task->esp0 = esp;
	task->esp = esp;
	task->cr3 = NULL;
	task->state = TASK_READY;

	task_add_to_list(ready, task);

	/* add eip to stack */
	if (seteip != NULL) {

		task->esp -= 4;
		*(uint32_t *)task->esp = (uint32_t)seteip;
		task->esp -= 16;
	}

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
