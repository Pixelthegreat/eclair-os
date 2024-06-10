#include <stdint.h>
#include <e.clair/io/port.h>

/* output byte */
extern void port_outb(uint16_t port, uint8_t data) {

	__asm__("outb %%al, %%dx": : "a"(data), "d"(port));
}

/* input byte */
extern uint8_t port_inb(uint16_t port) {

	uint8_t res;
	__asm__("inb %%dx, %%al": "=a"(res): "d"(port));
	return res;
}

/* output word */
extern void port_outw(uint16_t port, uint16_t data) {

	__asm__("outw %%ax, %%dx": : "a"(data), "d"(port));
}

/* input word */
extern uint16_t port_inw(uint16_t port) {

	uint16_t res;
	__asm__("inw %%dx, %%ax": "=a"(res): "d"(port));
	return res;
}
