#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/idt.h>
#include <kernel/tty.h>
#include <kernel/boot.h>
#include <kernel/panic.h>
#include <kernel/elf.h>
#include <kernel/mm/gdt.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/device.h>
#include <kernel/vfs/fs.h>
#include <kernel/vfs/devfs.h>
#include <kernel/fs/mbr.h>
#include <kernel/task.h>

extern void _kernel_end();

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
	mbr_fs_mount_root();
	devfs_init();
	task_init();
	elf_load_task("/bin/init");

	while (true) {

		task_cleanup();
		device_update();
		asm volatile("hlt");
	}
}
