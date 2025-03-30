#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/io/port.h>
#include <kernel/vfs/fs.h>
#include <kernel/vfs/devfs.h>
#include <kernel/driver/device.h>
#include <kernel/driver/uart.h>

static const uint16_t ports[UART_COM_COUNT] = {
	UART_PORT_COM1,
	UART_PORT_COM2,
	UART_PORT_COM3,
	UART_PORT_COM4,
};
static const char *names[UART_COM_COUNT] = {
	"UART COM1",
	"UART COM2",
	"UART COM3",
	"UART COM4",
};
static device_t *bus;
static device_t *devs[UART_COM_COUNT];
static fs_node_t *nodes[UART_COM_COUNT];

static uart_com_t init = 0;
static uart_com_t first = UART_COM_COUNT;

/* write fs */
static kssize_t write_fs(fs_node_t *dev, uint32_t offset, size_t nbytes, uint8_t *buf) {

	uart_write((uart_com_t)dev->impl, buf, nbytes);
	return (kssize_t)nbytes;
}

/* initialize uart */
extern void uart_init(uart_com_t bits, uint32_t rate) {

	if (init || !bits) return;

	bus = device_bus_new("UART");

	for (uart_com_t i = 0; i < UART_COM_COUNT; i++) {

		uart_com_t bit = (1 << i);
		if (bits & bit) {

			uint16_t port = ports[i];
			uint8_t baud = (uint8_t)UART_BAUD_RATE(rate);

			/* configure uart */
			port_outb(port+UART_PORT_FCR, 0); /* disable fifo */

			port_outb(port+UART_PORT_LCR, 0x80); /* unlock divisor */
			port_outb(port+UART_PORT_DLL, baud); /* baud rate (low byte) */
			port_outb(port+UART_PORT_DLH, 0); /* buad rate (high byte) */
			port_outb(port+UART_PORT_LCR, 0x03); /* lock divisor */
			port_outb(port+UART_PORT_MCR, 0);
			port_outb(port+UART_PORT_IER, 0x01); /* enable receive interrupts */

			/* fail */
			if (port_inb(port+UART_PORT_LSR) == UART_STATUS_FAIL) {

				kprintf(LOG_WARNING, "Failed to initialize UART (port 0x%x)", (uint32_t)port);
				return;
			}
			init |= bit;
			if (first >= UART_COM_COUNT) first = i;

			/* create driver device */
			devs[i] = device_terminal_new(names[i]);
			devs[i]->impl = (uint32_t)i;
			device_bus_add(bus, devs[i]);

			nodes[i] = fs_node_new(NULL, FS_CHARDEVICE);
			nodes[i]->write = write_fs;
			nodes[i]->impl = (uint32_t)i;

			/* write initial message */
			uart_writes(i, "Initialized UART\n");
		}
	}
}

/* initialize tty device */
extern void uart_init_tty(void) {

	if (first < UART_COM_COUNT) tty_add_device(nodes[first]);
}

/* initialize vfs nodes */
extern void uart_init_devfs(void) {

	for (uart_com_t i = 0; i < UART_COM_COUNT; i++) {
		if (init & (1 << i))
			devfs_add_node("uart", nodes[i]);
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

		uint16_t port = ports[com];
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
