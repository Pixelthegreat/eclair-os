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

	tty_printf("Hello, world!\n");
}
