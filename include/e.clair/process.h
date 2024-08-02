#ifndef ECLAIR_PROCESS_H
#define ECLAIR_PROCESS_H

#include <e.clair/types.h>
#include <e.clair/mm/paging.h>

/* process states */
#define PROCESS_STATE_READY 1
#define PROCESS_STATE_RUNNING 2
#define PROCESS_STATE_SLEEPING 3

/* process */
typedef struct process {
	void *esp; /* stack position */
	void *esp0; /* stack top */
	page_dir_entry_t *pagedir; /* page directory */
	struct process *next; /* next task in chain */
	uint32_t state; /* state of process */
} process_t;

extern process_t *process_active;

/* functions */
extern void process_init(void); /* initialize */
extern process_t *process_new(void *esp, void *seteip); /* create new process */
extern void process_start(void); /* start process irq */
extern void process_switch(process_t *proc); /* switch processes */
extern void process_schedule(void); /* schedule process */

#endif /* ECLAIR_PROCESS_H */
