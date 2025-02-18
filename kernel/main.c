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

#define SBUFSZ 512
static char sbuf[SBUFSZ];
static char sbuf2[SBUFSZ];
static char sbuf3[SBUFSZ];

static task_t *test_task;
static task_t *other_task;
static task_t *alt_task;
static fs_node_t *ttydev;

static void test_func();
static void other_func();
static void alt_func();

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

	/* draw text */
	fb_color_t color = {0xff, 0xff, 0xff};
	fb_text(20, 20, "Hello, world!", color);

	task_init();

	ttydev = tty_get_device(0);

	test_task = task_new(sbuf+SBUFSZ, test_func);
	other_task = task_new(sbuf2+SBUFSZ, other_func);
	alt_task = task_new(sbuf3+SBUFSZ, alt_func);

	char buf[32];
	while (true) {

		//asm volatile("hlt");
		fs_read(ttydev, 0, 32, buf);
	}
}

static void test_func() {

	task_unlockcli();

	while (true) {
		
		tty_printf("\e[31mHello, world?\e[39m\n");

		task_nano_sleep(500000000);
	}
}

static void other_func() {

	task_unlockcli();

	while (true) {

		tty_printf("\e[32mHello, world!\e[39m\n");

		task_sleep(2);
	}
}

static void alt_func() {

	task_unlockcli();

	while (true) {

		tty_printf("\e[33mHello, world.\e[39m\n");

		task_sleep(4);
	}
}
