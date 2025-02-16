#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/string.h>
#include <kernel/io/port.h>
#include <kernel/driver/device.h>
#include <kernel/driver/uart.h>

static const uint16_t PORTS[] = {
	UART_PORT_COM1,
	UART_PORT_COM2,
	UART_PORT_COM3,
	UART_PORT_COM4,
};

static uart_com_t init = 0;

/* initialize uart */
extern void uart_init(uart_com_t bits, uint32_t rate) {

	if (init) return;

	for (uart_com_t i = 0; i < UART_COM_COUNT; i++) {

		uart_com_t bit = (1 << i);
		if (bits & bit) {

			uint16_t port = PORTS[i];
			uint8_t baud = (uint8_t)UART_BAUD_RATE(rate);

			/* configure uart */
			port_outb(port+UART_PORT_FCR, 0); /* disable fifo */

			port_outb(port+UART_PORT_LCR, 0x80); /* unlock divisor */
			port_outb(port+UART_PORT_DLL, baud); /* baud rate (low byte) */
			port_outb(port+UART_PORT_DLH, 0); /* buad rate (high byte) */
			port_outb(port+UART_PORT_LCR, 0x03); /* unlock divisor */
			port_outb(port+UART_PORT_MCR, 0);
			port_outb(port+UART_PORT_IER, 0x01); /* enable receive interrupts */

			/* fail */
			if (port_inb(port+UART_PORT_LSR) == UART_STATUS_FAIL) {

				tty_printf("Failed to initialize UART (port 0x%x)\n", (uint32_t)port);
				return;
			}
			init |= bit;

			/* write initial message */
			uart_writes(i, "Initialized UART\n");
		}
	}
}

/* get initialized com ports */
extern uart_com_t uart_is_init(void) {

	return init;
}

/* write character */
extern void uart_writec(uart_com_t com, char c) {

	uart_com_t bit = (1 << com);
	if (init & bit) {

		uint16_t port = PORTS[com];
		int i;
		for (i = 0; i < 1048576 && !UART_STATUS_READY(port_inb(port+UART_PORT_LSR)); i++)
			port_outb(0x80, 0);
		port_outb(port+UART_PORT_THR, c);
	}
}

/* write data */
extern void uart_write(uart_com_t com, const void *data, size_t sz) {

	for (size_t i = 0; i < sz; i++) {

		char c = ((const char *)data)[i];
		uart_writec(com, c);

		if (c == '\n') uart_writec(com, '\r'); /* carriage return */
	}
}

/* write string */
extern void uart_writes(uart_com_t com, const char *str) {

	uart_write(com, str, strlen(str));
}
