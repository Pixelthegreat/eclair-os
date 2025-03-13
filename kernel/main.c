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

static void func1();
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

	//device_t *root = device_find_root();
	//fs_node_t *node = mbr_fs_probe(root, mbr_get_table(root));
	fs_node_t *node = mbr_fs_mount_root();
	//if (node) {

	//	node = fs_finddir(node, "hello.txt");
	//	if (node) {

	//		const char *msg = "We, the people of the United States, in order to form a more perfect union, establish justice, and ensure domestic tranquility, provide for the common defense, promote the general welfare and secure the blessings of liberty to ourselves and our prosperity, do ordain and establish this constitution for the United States of America.";
	//		size_t len = strlen(msg);
	//		size_t count = 16 * (1024 / len);

	//		fs_open(node, FS_WRITE);
	//		for (size_t i = 0; i < count; i++) {
	//			
	//			kssize_t nbytes = fs_write(node, node->len, len, (void *)msg);
	//			kprintf(LOG_INFO, "%d: %d", (int)i, nbytes);
	//		}
	//		fs_close(node);
	//	}
	//}

	//return;
	task_init();

	task1 = task_new(NULL, func1);
	task2 = task_new(NULL, func2);
	task3 = task_new(NULL, func3);

	while (true) {

		task_cleanup();
		device_update();
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
