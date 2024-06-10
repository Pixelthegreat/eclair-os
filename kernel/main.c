#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <e.clair/mm/gdt.h>
#include <e.clair/tty.h>

extern void kernel_main() {

	gdt_init();

	/* print stuff */
	tty_init();
	tty_printf("higher half: %x\n", (uint32_t)kernel_main);
}
