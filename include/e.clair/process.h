#ifndef ECLAIR_PROCESS_H
#define ECLAIR_PROCESS_H

#include <e.clair/types.h>

/* process */
typedef struct process {
	void *esp; /* stack pointer */
	void *esp0; /* stack top */
	void *pagedir; /* page directory */
	struct process *prev; /* previous process */
	struct process *next; /* next process */
} process_t;

extern process_t *process_active; /* active process block */

/* functions */
extern void process_init(void); /* initialize process subsystem */
extern process_t *process_new(void *esp, void *seteip); /* create new process */
extern process_t *process_get_active(void); /* get active process */
extern void process_switch(process_t *next); /* switch process (assembly) */

#endif /* ECLAIR_PROCESS_H */
