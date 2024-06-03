static void cls(void) {

	char *vmem = (char *)0xb8000;
	for (int i = 0; i < 25 * 80; i++) *vmem++ = 0;
}

static void printk(const char *s) {

	char *vmem = (char *)0xb8000;
	while (*s) {

		*vmem++ = *s++;
		*vmem++ = 0xc;
	}
}

extern void kernel_main() {

	cls();
	printk("E.Clair kernel");
}
