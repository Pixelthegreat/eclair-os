#ifndef ECLAIR_PROCESS_H
#define ECLAIR_PROCESS_H

#include <e.clair/types.h>
#include <e.clair/mm/paging.h>
#include <e.clair/driver/pit.h>

/* process states */
#define PROCESS_STATE_READY 1
#define PROCESS_STATE_RUNNING 2
#define PROCESS_STATE_SLEEPING 3
#define PROCESS_STATE_WAITING 4

struct process;

/* resource semaphore */
typedef struct resource {
	void *data; /* resource pointer */
	bool used; /* used by process */
	int refcnt; /* reference count */
	int maxcnt; /* maximum reference count */
	struct resource *prev; /* previous resource */
	struct resource *next; /* next resource */
	struct process *queue[]; /* queue of processes */
} resource_t;

/* signals */
typedef enum process_signal {
	PROCESS_SIGNAL_NONE = 0,

	PROCESS_SIGNAL_INT,

	PROCESS_SIGNAL_COUNT,
} process_signal_t;

/* process */
typedef struct process {
	void *esp; /* stack position */
	void *esp0; /* stack top */
	page_dir_entry_t *pagedir; /* page directory */
	struct process *next; /* next task in chain */
	uint32_t state; /* state of process */
	uint64_t wakeup; /* wake up time */
	uint32_t nticks; /* length of time slice */
	resource_t *waitres; /* waiting resource */
	uint32_t signal; /* called signal */
	void *ret; /* return address for signals */
	void *handlers[PROCESS_SIGNAL_COUNT]; /* signal handlers */
} process_t;

extern process_t *process_active;

/* functions */
extern void process_init(void); /* initialize */
extern process_t *process_new(void *esp, void *seteip); /* create new process */
extern void process_start(void); /* start process irq */
extern void process_lock_cli(void); /* lock interrupts */
extern void process_unlock_cli(void); /* unlock interrupts */
extern void process_lock_ts(void); /* lock task switches */
extern void process_unlock_ts(void); /* unlock task switches */
extern void process_switch(process_t *proc); /* switch processes */
extern process_t *process_get_next_ready(process_t *start); /* get next ready process */
extern void process_schedule(void); /* schedule process */
extern void process_block(uint32_t state); /* block active process */
extern void process_unblock(process_t *proc); /* unblock process */
extern void process_nano_sleep_until(uint64_t time); /* wait until time */
extern void process_nano_sleep(uint64_t time); /* wait for time */
extern void process_wake_up(void); /* wake up sleepers */
extern void process_acquire_resource(void *data); /* acquire resource */
extern void process_release_resource(void); /* release current resource */
extern void process_send_signal(process_t *proc, uint32_t signal); /* send signal to process */
extern void process_signal(void); /* assembly */
extern void process_handle_signal(void); /* call appropriate signal handler */

/* schedule next interrupt */
static inline void process_set_irq(void) {

	pit_set_channel(PIT_CHANNEL0, process_active->nticks & 0xff);
	pit_set_channel(PIT_CHANNEL0, (process_active->nticks >> 8) & 0xff);
}

#endif /* ECLAIR_PROCESS_H */
