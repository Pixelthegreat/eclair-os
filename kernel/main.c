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
#include <e.clair/driver/pit.h>

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

	/* hello */
	for (int i = 0; i < 8; i++) {
		for (int j = 1; j < 3; j++) {

			tty_printf("\x1b[%d;%dmHello, world!   ", j, i+30);
		}
		tty_print("\n");
	}
	tty_print("\x1b[0m\n");

	/* cause page fault */
	*(uint32_t *)NULL = 0;
}
