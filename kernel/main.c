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

#define BUFSZ 512
static char buf[BUFSZ];

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

	device_t *root = device_find_root();
	if (!root) return;

	mbr_t *mbr = mbr_get_table(root);
	if (!mbr) return;

	fs_node_t *node = mbr_fs_probe(root, mbr);
	if (!node) return;

	/* hello */
	fs_node_t *hello = fs_finddir(node, "hello.txt");
	if (!hello) return;

	fs_open(hello, FS_READ);

	ssize_t nread = fs_read(hello, 0, BUFSZ, buf);
	buf[26] = 0;

	tty_printf("%s\nBytes read: %d\n", buf, nread);

	fs_close(hello);
}
