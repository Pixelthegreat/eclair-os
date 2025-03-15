#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/idt.h>
#include <kernel/tty.h>
#include <kernel/boot.h>
#include <kernel/panic.h>
#include <kernel/mm/gdt.h>
#include <kernel/mm/paging.h>
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

static void func2();
static void func3();

extern void kernel_main() {

	gdt_init();
	idt_init();
	idt_enable();
	page_init();
	boot_init();
	page_init_top();
	task_init_memory();
	heap_init();
	fs_init();
	tty_init();
	device_init();
	ttydev = tty_get_device(0);
	mbr_fs_mount_root();
	task_init();

	task1 = task_new(NULL, task_entry);
	task2 = task_new(NULL, func2);
	task3 = task_new(NULL, func3);

	while (true) {

		task_cleanup();
		device_update();
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
