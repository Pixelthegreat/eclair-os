#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/idt.h>
#include <kernel/tty.h>
#include <kernel/boot.h>
#include <kernel/panic.h>
#include <kernel/mm/gdt.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/device.h>
#include <kernel/vfs/fs.h>
#include <kernel/vfs/devfs.h>
#include <kernel/fs/mbr.h>
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
	devfs_init();
	fs_node_print(fs_root);
	task_init();

	//task1 = task_new(NULL, task_entry);
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

	int fstdin = task_fs_open("/dev/tty0", FS_READ, 0);
	int fstdout = task_fs_open("/dev/tty0", FS_WRITE, 0);

	char buf[32];

	task_fs_write(fstdout, "What is your name? ", 19);
	task_fs_read(fstdin, buf, 32);

	while (true) {

		asm volatile("hlt");
	}
}

static void func3() {

	task_unlockcli();

	int ftty = task_fs_open("/dev/tty0", FS_READ | FS_WRITE, 0);

	char buf[32];

	task_fs_write(ftty, "What isn't your name? ", 22);
	task_fs_read(ftty, buf, 32);

	while (true) {

		asm volatile("hlt");
	}
}
