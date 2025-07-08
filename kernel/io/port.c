/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/io/port.h>

/* output byte */
extern void port_outb(uint16_t port, uint8_t data) {

	asm volatile("outb %%al, %%dx": : "a"(data), "d"(port));
}

/* input byte */
extern uint8_t port_inb(uint16_t port) {

	uint8_t res;
	asm volatile("inb %%dx, %%al": "=a"(res): "d"(port));
	return res;
}

/* output word */
extern void port_outw(uint16_t port, uint16_t data) {

	asm volatile("outw %%ax, %%dx": : "a"(data), "d"(port));
}

/* input word */
extern uint16_t port_inw(uint16_t port) {

	uint16_t res;
	asm volatile("inw %%dx, %%ax": "=a"(res): "d"(port));
	return res;
}

/* output dword */
extern void port_outd(uint16_t port, uint32_t data) {

	asm volatile("outl %%eax, %%dx": : "a"(data), "d"(port));
}

/* input dword */
extern uint32_t port_ind(uint16_t port) {

	uint32_t res;
	asm volatile("inl %%dx, %%eax": "=a"(res): "d"(port));
	return res;
}
