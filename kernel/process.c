#include <e.clair/types.h>
#include <e.clair/tty.h>
#include <e.clair/mm/paging.h>
#include <e.clair/mm/heap.h>
#include <e.clair/process.h>

static process_t *first = NULL;
static process_t *last = NULL;

process_t *process_active = NULL;

extern void (*kernel_stack_top)();

/* initialize process subsystem */
extern void process_init(void) {

	process_t *proc = process_new(kernel_stack_top, NULL);
	proc->pagedir = page_get_directory();
	process_active = proc;
}

/* create new process */
extern process_t *process_new(void *esp, void *seteip) {

	process_t *proc = (process_t *)kmalloc(sizeof(process_t));
	proc->esp = esp;
	proc->esp0 = esp;
	proc->pagedir = NULL;

	/* set return address */
	if (seteip) {

		proc->esp -= 4;
		*(void **)proc->esp = seteip;
		proc->esp -= 16; /* include registers to be popped */
	}

	/* add to list */
	proc->prev = last;
	if (last) last->next = proc;
	if (!first) first = proc;
	last = proc;

	return proc;
}

/* get active process */
extern process_t *process_get_active(void) {

	return process_active;
}
