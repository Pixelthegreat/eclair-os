#include <e.clair/types.h>
#include <e.clair/mm/paging.h>
#include <e.clair/mm/heap.h>
#include <e.clair/driver/pit.h>
#include <e.clair/process.h>

#define NTICKS 10

process_t *process_active = NULL; /* active process */

extern void kernel_stack_top();

/* pit callback */
static void process_pit_callback(idt_regs_t *regs) {

	idt_send_eoi();

	pit_set_channel(PIT_CHANNEL0, NTICKS & 0xff);
	pit_set_channel(PIT_CHANNEL0, (NTICKS >> 8) & 0xff);

	process_schedule();
}

/* initialize */
extern void process_init(void) {

	process_active = process_new(kernel_stack_top, NULL);
	process_active->pagedir = page_get_directory();
}

/* create new process */
extern process_t *process_new(void *esp, void *seteip) {

	process_t *proc = (process_t *)kmalloc(sizeof(process_t));

	proc->esp = esp;
	proc->esp0 = esp;
	proc->pagedir = NULL;
	proc->next = NULL;
	proc->state = PROCESS_STATE_READY;

	/* push return address */
	if (seteip) {

		proc->esp -= 4;
		*(void **)proc->esp = seteip;
		proc->esp -= 16;
	}

	return proc;
}

/* start process irq */
extern void process_start(void) {

	pit_set_callback(process_pit_callback);

	pit_set_channel(PIT_CHANNEL0, NTICKS & 0xff);
	pit_set_channel(PIT_CHANNEL0, (NTICKS >> 8) & 0xff);
}


/* schedule */
extern void process_schedule(void) {

	process_switch(process_active->next);
}
