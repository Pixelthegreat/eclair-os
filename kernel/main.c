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

extern void kernel_main() {

	gdt_init();
	idt_init();
	idt_enable();
	tty_init();
	page_init();
	heap_init();
	device_init();
	fs_init();

	/* hello */
	device_t *root = device_find_root();

	uint16_t *buf = (uint16_t *)kmalloc(512);
	device_storage_read(root, 0, 1, buf);

	tty_printf("Boot signature: 0x%x\n", buf[255]);

	buf[255] = 0xccbb;
	device_storage_write(root, 0, 1, buf);
}
