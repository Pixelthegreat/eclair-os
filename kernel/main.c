#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/mm/gdt.h>
#include <kernel/idt.h>
#include <kernel/mm/paging.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/device.h>
#include <kernel/vfs/fs.h>
#include <kernel/fs/mbr.h>
#include <kernel/fs/ext2.h>
#include <kernel/task.h>

#define SBUFSZ 512
static char sbuf[SBUFSZ];

static task_t *test_task;
static fs_node_t *ttydev;

static void test_func();

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
	task_init();

	ttydev = tty_get_device();

	test_task = task_new(sbuf+SBUFSZ, test_func);
	test_task->cr3 = ktask->cr3;

	while (true) {
		
		tty_printf("\e[32mHello, world!\e[39m\n");

		for (int i = 0; i < 6000000; i++);

		task_lockcli();
		task_schedule();
		task_unlockcli();
	}
}

static void test_func() {

	task_unlockcli();

	while (true) {
		
		tty_printf("\e[31mHello, world?\e[39m\n");

		task_nano_sleep(500000000);
	}
}
