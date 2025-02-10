#ifndef ECLAIR_TASK_H
#define ECLAIR_TASK_H

#include <kernel/types.h>

#define TASK_READY 0
#define TASK_RUNNING 1
#define TASK_PAUSED 2
#define TASK_SLEEPING 3

#define TASK_NSTATES 4

/* task control block */
typedef struct task {
	void *esp0; /* kernel stack top */
	void *esp; /* current stack position */
	void *cr3; /* address space */
	struct task *prev; /* previous task */
	struct task *next; /* next task */
	uint32_t state; /* task state */
	uint64_t waketime; /* wake up time for sleeping task */
	uint32_t nticks; /* number of ticks left */
} task_t;

extern task_t *ktask; /* base kernel task */
extern task_t *task_active; /* active task */

extern uint32_t task_nlockpost; /* number of task switch locks */
extern uint32_t task_postponed; /* postponed task switches */

/* functions */
extern void task_init(void); /* initialize multitasking */
extern task_t *task_new(void *esp, void *seteip); /* create task */
extern void task_switch(task_t *task); /* switch to next task */
extern void task_schedule(void); /* schedule next task */
extern void task_lockcli(void); /* lock interrupts */
extern void task_unlockcli(void); /* unlock interrupts */
extern void task_lockpost(void); /* lock task switches */
extern void task_unlockpost(void); /* unlock task switches */
extern void task_block(uint32_t reason); /* block current task */
extern void task_unblock(task_t *task); /* unblock task */
extern void task_nano_sleep_until(uint64_t waketime); /* sleep in nanoseconds until */
extern void task_nano_sleep(uint64_t ns); /* sleep in nanoseconds */
extern void task_sleep(uint32_t s); /* sleep in seconds */

#endif /* ECLAIR_TASK_H */
