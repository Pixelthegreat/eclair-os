/* credit for most of this driver goes to the xv6 project */
#ifndef ECLAIR_DRIVER_UART_H
#define ECLAIR_DRIVER_UART_H

#include <kernel/types.h>

typedef enum uart_com {
	UART_COM1 = 0,
	UART_COM2,
	UART_COM3,
	UART_COM4,
	UART_COM_COUNT,

	UART_COM1_BIT = 0x1,
	UART_COM2_BIT = 0x2,
	UART_COM3_BIT = 0x4,
	UART_COM4_BIT = 0x8,
} uart_com_t;

#define UART_PORT_COM1 0x3f8
#define UART_PORT_COM2 0x2f8
#define UART_PORT_COM3 0x3e8
#define UART_PORT_COM4 0x2e8

#define UART_BAUD_RATE(r) (115200/(r))
#define UART_DEFAULT_BAUD_RATE 9600

#define UART_PORT_THR 0
#define UART_PORT_RBR 0
#define UART_PORT_DLL 0
#define UART_PORT_IER 1
#define UART_PORT_DLH 1
#define UART_PORT_IIR 2
#define UART_PORT_FCR 2
#define UART_PORT_LCR 3
#define UART_PORT_MCR 4
#define UART_PORT_LSR 5
#define UART_PORT_MSR 6
#define UART_PORT_SR 7

#define UART_STATUS_FAIL 0xff
#define UART_STATUS_READY(s) ((s) & 0x20)

/* functions */
extern void uart_init(uart_com_t bits, uint32_t rate); /* initialize uart */
extern uart_com_t uart_is_init(void); /* get initialized com ports */
extern void uart_writec(uart_com_t com, char c); /* write character */
extern void uart_write(uart_com_t com, const void *data, size_t sz); /* write data */
extern void uart_writes(uart_com_t com, const char *str); /* write string */

#endif /* ECLAIR_DRIVER_UART_H */
