#include <e.clair/types.h>
#include <e.clair/mm/gdt.h>
#include <e.clair/idt.h>
#include <e.clair/mm/paging.h>
#include <e.clair/driver/ps2.h>
#include <e.clair/tty.h>
#include <e.clair/multiboot.h>

extern void kernel_main() {

	gdt_init();
	idt_init();
	tty_init();
	ps2_init();
	idt_enable();
	page_init();
	multiboot_init();

	/* print stuff */
	multiboot_saved_info_t *saved = multiboot_get_saved_info();
	tty_printf("command line: %s\nmemory lower: 0x%x\nmemory upper: 0x%x\nbios boot device: 0x%x\nboot partition: %d\nboot sub-partition: %d\n", saved->cmdline, saved->memlow, saved->memup, saved->biosdev, saved->part, saved->subpart);
}
