#ifndef ECLAIR_DRIVER_UHCI_H
#define ECLAIR_DRIVER_UHCI_H

#include <kernel/types.h>
#include <kernel/driver/device.h>

#define UHCI_CLASS 0xc
#define UHCI_SUBCLASS 0x3
#define UHCI_PROGIF 0x0

#define UHCI_PCIREG_LEGSUP 0xc0
#define UHCI_LEGSUP_PIRQ 0x2000

#define UHCI_REG_USBCMD 0x00
#define UHCI_REG_USBSTS 0x02
#define UHCI_REG_USBINTR 0x04
#define UHCI_REG_FRNUM 0x06
#define UHCI_REG_FRBASEADD 0x08
#define UHCI_REG_SOFMOD 0x0c
#define UHCI_REG_PORTSC1 0x10
#define UHCI_REG_PORTSC2 0x12

/* command register */
#define UHCI_USBCMD_RUN 0x1
#define UHCI_USBCMD_HCRESET 0x2
#define UHCI_USBCMD_GRESET 0x4
#define UHCI_USBCMD_GSUSPEND 0x8
#define UHCI_USBCMD_GRESUME 0x10
#define UHCI_USBCMD_SDEBUG 0x20
#define UHCI_USBCMD_CFGFLAG 0x40
#define UHCI_USBCMD_MAXPKSZ 0x80

/* status register */
#define UHCI_USBSTS_INT 0x1
#define UHCI_USBSTS_EINT 0x2
#define UHCI_USBSTS_RDET 0x4
#define UHCI_USBSTS_SYSERR 0x8
#define UHCI_USBSTS_PROCERR 0x10
#define UHCI_USBSTS_HLT 0x20

/* interrupt enable register */
#define UHCI_USBINTR_TMCRC 0x1
#define UHCI_USBINTR_RESUME 0x2
#define UHCI_USBINTR_COMPTFR 0x4
#define UHCI_USBINTR_SHORTPK 0x8

/* frame list entry */
#define UHCI_FRAME_EMPTY 0x1
#define UHCI_FRAME_STYPE 0x2

/* port registers */
#define UHCI_PORT_CNSTATUS 0x1
#define UHCI_PORT_CNSTATCHG 0x2
#define UHCI_PORT_DEVENABLE 0x4
#define UHCI_PORT_PORTENBCHG 0x8
#define UHCI_PORT_LSTATUS 0x30
#define UHCI_PORT_RESUME 0x40
#define UHCI_PORT_PRESENT 0x80
#define UHCI_PORT_LOSPEED 0x100
#define UHCI_PORT_RESET 0x200

/* uhci info */
typedef struct uhci_info {
	uint16_t iobase; /* base of io ports */
	uint16_t iosize; /* io range size */
	uint32_t *flist; /* frame list */
} uhci_info_t;

/* functions */
extern uint16_t uhci_inw(device_t *dev, uint16_t port); /* read word from uhci port */
extern void uhci_outw(device_t *dev, uint16_t port, uint16_t data); /* write word to uhci port */
extern uint32_t uhci_ind(device_t *dev, uint16_t port); /* read dword from uhci port */
extern void uhci_outd(device_t *dev, uint16_t port, uint32_t data); /* write dword to uhci port */
extern void uhci_register(void); /* register pci driver */

#endif /* ECLAIR_DRIVER_UHCI_H */
