#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/mm/gdt.h>
#include <kernel/idt.h>
#include <kernel/mm/paging.h>
#include <kernel/tty.h>
#include <kernel/boot.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/fb.h>
#include <kernel/driver/device.h>
#include <kernel/vfs/fs.h>
#include <kernel/fs/mbr.h>
#include <kernel/fs/ext2.h>
#include <kernel/task.h>

static task_t *task1;
static task_t *task2;
static task_t *task3;

static fs_node_t *ttydev = NULL;

static void func1();
static void func2();
static void func3();

extern void kernel_main() {

	gdt_init();
	idt_init();
	idt_enable();
	page_init();
	boot_init();
	task_init_memory();
	heap_init();
	fs_init();
	tty_init();
	device_init();
	tty_printf("0x%x\n", boot_data_info);

	ttydev = tty_get_device(0);
	task_init();
	tty_printf("C\n");

	task1 = task_new(NULL, func1);
	task2 = task_new(NULL, func2);
	task3 = task_new(NULL, func3);

	while (true) {

		task_cleanup();
		asm volatile("hlt");
	}
}

static void func1() {

	task_unlockcli();

	char buf[32];
	while (true) {

		task_acquire(ttydev);
		tty_printf("First task\n");
		fs_read(ttydev, 0, 32, buf);
		task_release();

		asm volatile("hlt");
	}
}

static void func2() {

	task_unlockcli();

	char buf[32];
	while (true) {

		task_acquire(ttydev);
		tty_printf("Second task\n");
		fs_read(ttydev, 0, 32, buf);
		task_release();

		asm volatile("hlt");
	}
}

static void func3() {

	task_unlockcli();

	char buf[32];
	while (true) {

		task_acquire(ttydev);
		tty_printf("Third task\n");
		fs_read(ttydev, 0, 32, buf);
		task_release();

		asm volatile("hlt");
	}
}
