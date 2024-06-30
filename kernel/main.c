#include <e.clair/types.h>
#include <e.clair/mm/gdt.h>
#include <e.clair/idt.h>
#include <e.clair/mm/paging.h>
#include <e.clair/driver/ps2.h>
#include <e.clair/tty.h>
#include <e.clair/multiboot.h>
#include <e.clair/heap.h>

extern void kernel_main() {

	gdt_init();
	idt_init();
	idt_enable();
	tty_init();
	ps2_init();
	page_init();
	multiboot_init();
	heap_init();

	/* test kmalloc */
	char *buf0 = kmalloc(128);

	tty_printf("A\n");
	heap_print();

	char *buf1 = kmalloc(128);

	tty_printf("B\n");
	heap_print();

	uint8_t *buf2 = kmalloc(4096);

	tty_printf("C\n");
	heap_print();

	*buf0 = 0x41;
	*(buf0+1) = 0;

	*buf1 = 0x42;
	*(buf1+1) = 0;

	*(buf2+4095) = 0x43;

	tty_printf("%s\n%s\n0x%x\n", buf0, buf1, *(buf2+4095));

	kfree(buf1);

	tty_printf("D\n");
	heap_print();

	kfree(buf0);

	tty_printf("E\n");
	heap_print();

	buf0 = kmalloc(280);
	
	tty_printf("F\n");
	heap_print();

	kfree(buf2);

	tty_printf("G\n");
	heap_print();

	*buf0 = 0x41;
	*(buf0+1) = 0;
	tty_printf("%s\n", buf0);
}
