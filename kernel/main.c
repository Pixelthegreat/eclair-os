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
	tty_init();
	page_init();
	heap_init();
	device_init();
	fs_init();

	device_t *root = device_find_root();
	if (!root) return;

	mbr_t *mbr = mbr_get_table(root);
	if (!mbr) return;

	fs_node_t *node = mbr_fs_probe(root, mbr);
	if (!node) return;

	fs_node_t *bootdir = fs_finddir(node, "boot");
	if (bootdir) {

		fs_node_t *kernel = fs_finddir(bootdir, "e.clair");
		if (kernel) {

			tty_printf("kernel size: 0x%x\n", kernel->len);
		}
	}

	/* hello */
	tty_printf("hello, world!\n");
}
