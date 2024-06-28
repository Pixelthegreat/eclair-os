#ifndef ECLAIR_IO_PORT_H
#define ECLAIR_IO_PORT_H

#include <e.clair/types.h>

extern void port_outb(uint16_t port, uint8_t data);
extern uint8_t port_inb(uint16_t port);
extern void port_outw(uint16_t port, uint16_t data);
extern uint16_t port_inw(uint16_t port);

#endif /* ECLAIR_IO_PORT_H */
