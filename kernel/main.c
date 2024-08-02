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

static process_t *kproc = NULL;
static process_t *proc = NULL;

static void procfn();

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

	kproc->next = proc;
	proc->next = kproc;

	process_start();

	while (1) {
		asm("sti");

		tty_printf("A");
	}
}

static void procfn() {

	while (1) {
		asm("sti");

		tty_printf("B");
	}
}
