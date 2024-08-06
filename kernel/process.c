#include <e.clair/types.h>
#include <e.clair/string.h>
#include <e.clair/mm/paging.h>
#include <e.clair/mm/heap.h>
#include <e.clair/driver/pit.h>
#include <e.clair/driver/rtc.h>
#include <e.clair/process.h>

#define NTICKS 20

static uint32_t nlockcli = 0; /* number of interrupt locks */
static uint32_t nlockts = 0; /* number of task switch locks */

static resource_t *first_res = NULL; /* first resource in list */
static resource_t *last_res = NULL; /* last resource */

process_t *process_active = NULL; /* active process */
static process_t *idle = NULL; /* idle process */

extern void kernel_stack_top();

/* auto schedule next process */
static void process_pit_callback(idt_regs_t *regs) {

	process_wake_up();

	/* set next interrupt */
	idt_send_eoi();
	process_set_irq();

	process_schedule();

	/* signaled */
	process_lock_ts();
	if (process_active->signal) {
		process_active->ret = (void *)regs->eip;
		regs->eip = (uint32_t)process_signal;
	}
	process_unlock_ts();
}

/* initialize */
extern void process_init(void) {

	process_active = process_new(kernel_stack_top, NULL);

	process_active->pagedir = page_get_directory();
	process_active->state = PROCESS_STATE_RUNNING;

	idle = process_active;
}

/* create new process */
extern process_t *process_new(void *esp, void *seteip) {

	process_t *proc = (process_t *)kmalloc(sizeof(process_t));

	proc->esp = esp;
	proc->esp0 = esp;
	proc->pagedir = NULL;
	proc->next = NULL;
	proc->state = PROCESS_STATE_READY;
	proc->wakeup = 0;
	proc->nticks = NTICKS;
	proc->waitres = NULL;
	proc->signal = 0;
	proc->ret = NULL;
	memset(proc->handlers, 0, sizeof(proc->handlers));

	/* push return address */
	if (seteip) {

		proc->esp -= 4;
		*(void **)proc->esp = seteip;
		proc->esp -= 16;
	}

	return proc;
}

/* lock interrupts */
extern void process_lock_cli(void) {

	if (!(nlockcli++)) asm("cli");
}

/* unlock interrupts */
extern void process_unlock_cli(void) {

	if (!nlockcli || !(--nlockcli)) asm("sti");
}

/* lock task switches */
extern void process_lock_ts(void) {

	nlockts++;
}

/* unlock task switches */
extern void process_unlock_ts(void) {

	if (nlockts) nlockts--;
}

/* start process irq */
extern void process_start(void) {

	pit_set_callback(process_pit_callback);

	process_set_irq();
}

/* get next ready process */
extern process_t *process_get_next_ready(process_t *start) {

	if (!start) start = process_active;

	process_t *proc = start->next;
	while (proc && proc != process_active) {

		if (proc->state == PROCESS_STATE_READY)
			return proc;
		proc = proc->next;
	}
	return NULL;
}

/* schedule */
extern void process_schedule(void) {

	if (nlockts) return;

	/* get next ready process */
	process_t *ready = process_get_next_ready(NULL);
	if (!ready) return;

	/* if idle process, determine whether to use it */
	if (ready == idle) {

		ready = process_get_next_ready(ready);
		if (!ready) ready = idle;
	}

	process_switch(ready);
}

/* block active process */
extern void process_block(uint32_t state) {

	process_active->state = state;
	process_schedule();
}

/* unblock process */
extern void process_unblock(process_t *proc) {

	proc->state = PROCESS_STATE_READY;
}

/* wait until time */
extern void process_nano_sleep_until(uint64_t time) {

	process_active->wakeup = time;
	process_block(PROCESS_STATE_SLEEPING);
}

/* wait for time */
extern void process_nano_sleep(uint64_t time) {

	process_nano_sleep_until(rtc_ns + time);
}

/* wake up sleepers */
extern void process_wake_up(void) {

	process_t *proc = process_active->next;
	for (; proc && proc != process_active; proc = proc->next) {

		/* wake up */
		if (proc->state == PROCESS_STATE_SLEEPING && rtc_ns >= proc->wakeup)
			process_unblock(proc);
	}
}

/* acquire resource */
extern void process_acquire_resource(void *data) {

	process_lock_ts();

	resource_t *cur = first_res;
	while (cur) {

		if (cur->data == data)
			break;
		cur = cur->next;
	}

	/* create resource struct */
	if (!cur) {

		int maxcnt = 16;
		cur = (resource_t *)kmalloc(sizeof(resource_t) + (sizeof(process_t *) * maxcnt));

		cur->data = data;
		cur->used = false;
		cur->refcnt = 0;
		cur->maxcnt = maxcnt;
		cur->next = NULL;
		cur->prev = last_res;
		if (last_res) last_res->next = cur;
		if (!first_res) first_res = cur;
		last_res = cur;
	}

	/* block if waiting or if other processes are waiting */
	process_active->waitres = cur;
	if (cur->used) {

		if (cur->refcnt < cur->maxcnt)
			cur->queue[cur->refcnt++] = process_active;

		process_unlock_ts();
		process_block(PROCESS_STATE_WAITING);
		process_lock_ts();
	}

	cur->used = true;
	process_unlock_ts();
}

/* release current resource */
extern void process_release_resource(void) {

	resource_t *res = process_active->waitres;
	if (!res) return;

	process_lock_ts();

	res->used = false;
	process_active->waitres = NULL;

	/* unblock process */
	if (res->refcnt) {

		process_t *proc = res->queue[0];
		res->refcnt--;
		for (int i = 0; i < res->refcnt; i++)
			res->queue[i] = res->queue[i+1];
		process_unblock(proc);

		/* schedule */
		process_unlock_ts();
		process_schedule();
		process_lock_ts();
	}
	process_unlock_ts();
}

/* send signal to process */
extern void process_send_signal(process_t *proc, uint32_t signal) {

	if (proc->signal) return;

	proc->signal = signal;
}

/* call appropriate signal handler */
extern void process_handle_signal(void) {

	void (*sig)(int) = process_active->handlers[process_active->signal];
	if (!sig) return;

	sig(process_active->signal);
}
