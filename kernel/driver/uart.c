#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/string.h>
#include <kernel/io/port.h>
#include <kernel/vfs/fs.h>
#include <kernel/driver/device.h>
#include <kernel/driver/uart.h>

static const uint16_t PORTS[UART_COM_COUNT] = {
	UART_PORT_COM1,
	UART_PORT_COM2,
	UART_PORT_COM3,
	UART_PORT_COM4,
};
static const char *NAMES[UART_COM_COUNT] = {
	"UART COM1",
	"UART COM2",
	"UART COM3",
	"UART COM4",
};
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
			port_outb(port+UART_PORT_LCR, 0x03); /* lock divisor */
			port_outb(port+UART_PORT_MCR, 0);
			port_outb(port+UART_PORT_IER, 0x01); /* enable receive interrupts */

			/* fail */
			if (port_inb(port+UART_PORT_LSR) == UART_STATUS_FAIL) {

				tty_printf("Failed to initialize UART (port 0x%x)\n", (uint32_t)port);
				return;
			}
			init |= bit;
			if (first >= UART_COM_COUNT) first = i;

			/* create driver device */
			devs[i] = device_new(DEVICE_TYPE_CHAR, DEVICE_SUBTYPE_CHAR_UART, NAMES[i], sizeof(device_char_t));
			devs[i]->impl = (uint32_t)i;

			device_char_t *chardev = (device_char_t *)devs[i];

			chardev->s_ibuf = 0; chardev->e_ibuf = 0;
			chardev->s_obuf = 0; chardev->e_obuf = 0;
			chardev->flush = NULL;

			/* create vfs char device */
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

/* get initialized com ports */
extern uart_com_t uart_is_init(void) {

	return init % UART_COM_COUNT;
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
