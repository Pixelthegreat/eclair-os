#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/mm/gdt.h>
#include <kernel/idt.h>
#include <kernel/mm/paging.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>
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

#define TASK1_NLOOP 10
#define TASK2_NLOOP 5
#define TASK3_NLOOP 3

static void func1();
static void func2();
static void func3();

extern void kernel_main() {

	gdt_init();
	idt_init();
	idt_enable();
	page_init();
	multiboot_init();
	task_init_memory();
	heap_init();
	fs_init();
	tty_init();
	device_init();
	task_init();

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

	int count = 0;
	while (true) {
		
		tty_printf("\e[31mHello, world?\e[39m\n");

		task_nano_sleep(500000000);
		if (++count >= TASK1_NLOOP)
			task_terminate();
	}
}

static void func2() {

	task_unlockcli();

	int count = 0;
	while (true) {

		tty_printf("\e[32mHello, world!\e[39m\n");

		task_sleep(2);
		if (++count >= TASK2_NLOOP)
			task_terminate();
	}
}

static void func3() {

	task_unlockcli();

	int count = 0;
	while (true) {

		tty_printf("\e[33mHello, world.\e[39m\n");

		task_sleep(4);
		if (++count >= TASK3_NLOOP)
			task_terminate();
	}
}
