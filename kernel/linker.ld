ENTRY(_start)
SECTIONS {
	. = 0x100000;

	_kernel_start = .;
	.multiboot.data : { *(.multiboot.data) }
	.multiboot.text : { *(.multiboot.text) }

	. += 0xC0000000;
	.text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) { *(.text) }
	.data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000) {
		*(.data);
		_driverinfo_start = .;
		*(.driverinfo);
		_driverinfo_end = .;
	}
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000) { *(.rodata) }
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000) {
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}
	_kernel_end = .;
}
