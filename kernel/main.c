#include <e.clair/types.h>
#include <e.clair/string.h>
#include <e.clair/mm/gdt.h>
#include <e.clair/idt.h>
#include <e.clair/mm/paging.h>
#include <e.clair/tty.h>
#include <e.clair/multiboot.h>
#include <e.clair/mm/heap.h>
#include <e.clair/driver/device.h>
#include <e.clair/vfs/fs.h>
#include <e.clair/fs/mbr.h>
#include <e.clair/fs/ext2.h>
#include <e.clair/process.h>

#define BUFSZ 512
static char buf[BUFSZ];
static char buf2[BUFSZ];

static process_t *kproc = NULL;
static process_t *proc = NULL;
static process_t *proc2 = NULL;

static fs_node_t *ttydev = NULL;

static void procfn();
static void procsigfn(int _);
static void proc2fn();

extern void kernel_main() {

	gdt_init();
	idt_init();
	idt_enable();
	page_init();
	multiboot_init();
	heap_init();
	fs_init();
	tty_init();
	device_init();
	process_init();

	kproc = process_active;

	proc = process_new(buf+BUFSZ, procfn);
	proc->pagedir = kproc->pagedir;
	proc->handlers[PROCESS_SIGNAL_INT] = procsigfn;

	proc2 = process_new(buf2+BUFSZ, proc2fn);
	proc2->pagedir = proc->pagedir;

	kproc->next = proc;
	proc->next = proc2;
	proc2->next = kproc;

	ttydev = tty_get_device();

	process_start();
	process_unlock_cli();
}

static void procfn() {

	process_unlock_cli();
	while (1) {

		asm("hlt");
	}
}

static void procsigfn(int _) {

	process_acquire_resource(ttydev);
	tty_printf("Signal received\n");
	process_release_resource();
}

static void proc2fn() {

	process_unlock_cli();
	while (1) {

		process_nano_sleep(1000000000);
		process_send_signal(proc, PROCESS_SIGNAL_INT);
	}
}
