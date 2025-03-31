#ifndef ECLAIR_PCI_H
#define ECLAIR_PCI_H

#include <kernel/types.h>
#include <kernel/driver/device.h>

#define PCI_PORT_ADDR 0xcf8
#define PCI_PORT_DATA 0xcfc

#define PCI_ADDR(b,d,f,r) (((b) << 16) | ((d) << 11) | ((f) << 8) | (r) | 0x80000000)

#define PCI_REG_VENDORID 0x0
#define PCI_REG_DEVICEID 0x2
#define PCI_REG_STATUS 0x4
#define PCI_REG_COMMAND 0x6
#define PCI_REG_CLASS 0x8
#define PCI_REG_SUBCLASS 0x9
#define PCI_REG_PROGIF 0xa
#define PCI_REG_REVID 0xb
#define PCI_REG_BIST 0xc
#define PCI_REG_HTYPE 0xd
#define PCI_REG_LTIMER 0xe
#define PCI_REG_CLINESZ 0xf

#define PCI_HTYPE_MFUNC 0x80

#define PCI_HTYPE_GENERIC 0x0
#define PCI_HTYPE_PCI2PCI 0x1
#define PCI_HTYPE_PCI2CB 0x2

/* registers for generic devices */
#define PCI_GENERIC_BAR0 0x10
#define PCI_GENERIC_BAR1 0x14
#define PCI_GENERIC_BAR2 0x18
#define PCI_GENERIC_BAR3 0x1c
#define PCI_GENERIC_BAR4 0x20
#define PCI_GENERIC_BAR5 0x24
#define PCI_GENERIC_CBCISP 0x28
#define PCI_GENERIC_SUBSYSID 0x2c
#define PCI_GENERIC_SUBSYSVENDORID 0x30
#define PCI_GENERIC_CAPBP 0x37
#define PCI_GENERIC_MAXLATNC 0x3c
#define PCI_GENERIC_MINGRANT 0x3d
#define PCI_GENERIC_INTPIN 0x3e
#define PCI_GENERIC_INTLINE 0x3f

/* registers for pci to pci bridges */
#define PCI_PCI2PCI_BAR0 0x10
#define PCI_PCI2PCI_BAR1 0x14
#define PCI_PCI2PCI_LTIMER 0x18
#define PCI_PCI2PCI_NBUS1 0x19
#define PCI_PCI2PCI_NBUS2 0x1a
#define PCI_PCI2PCI_NBUS0 0x1b
#define PCI_PCI2PCI_STATUS 0x1c
#define PCI_PCI2PCI_IOLIM 0x1e
#define PCI_PCI2PCI_IOBASE 0x1f
#define PCI_PCI2PCI_MEMLIM 0x20
#define PCI_PCI2PCI_MEMBASE 0x22
#define PCI_PCI2PCI_PMEMLIM 0x24
#define PCI_PCI2PCI_PMEMBASE 0x26
#define PCI_PCI2PCI_PBASEHI 0x28
#define PCI_PCI2PCI_PLIMHI 0x2c
#define PCI_PCI2PCI_IOLIMHI 0x30
#define PCI_PCI2PCI_IOBASEHI 0x32
#define PCI_PCI2PCI_CAPBP 0x37
#define PCI_PCI2PCI_ROMBASE 0x38
#define PCI_PCI2PCI_BCTL 0x3c
#define PCI_PCI2PCI_INTPIN 0x3e
#define PCI_PCI2PCI_INTLINE 0x3f

/* registers for pci to cardbus bridges */
#define PCI_PCI2CB_CBSOCK 0x10
#define PCI_PCI2CB_STATUS 0x14
#define PCI_PCI2CB_CAPBP 0x17
#define PCI_PCI2CB_MBA0 0x1c
#define PCI_PCI2CB_MLIM0 0x20
#define PCI_PCI2CB_MBA1 0x24
#define PCI_PCI2CB_MLIM1 0x28
#define PCI_PCI2CB_IOBA0 0x2c
#define PCI_PCI2CB_IOLIM0 0x30
#define PCI_PCI2CB_IOBA1 0x34
#define PCI_PCI2CB_IOLIM1 0x38
#define PCI_PCI2CB_BCTL 0x3c
#define PCI_PCI2CB_INTPIN 0x3e
#define PCI_PCI2CB_INTLINE 0x3f
#define PCI_PCI2CB_SUBSYSVENDORID 0x40
#define PCI_PCI2CB_SUBSYSDEVID 0x42
#define PCI_PCI2CB_PCLBA 0x44

/* important class codes */
#define PCI_CLASS_BRIDGE 0x6

#define PCI_SUBCLASS_PCI2PCI_BRIDGE 0x4

/* pci driver */
typedef struct pci_device {
	bool present; /* present indicator for buses */
	device_t *device; /* kernel device */
	uint32_t nbus, ndev; /* pci device location */
	uint32_t func; /* available functions bitfield */
} pci_device_t;

typedef struct pci_bus {
	uint32_t nbus; /* bus number */
	device_t *bus; /* kernel bus device */
	pci_device_t devs[32]; /* pci devices */
} pci_bus_t;

typedef struct pci_driver {
	bool (*match)(uint32_t, uint32_t); /* match driver with device */
	device_t *(*init)(pci_device_t *); /* initialize driver with device */
	struct pci_driver *next; /* next driver in list */
} pci_driver_t;

/* functions */
extern uint8_t pci_inb(uint32_t addr, uint32_t offset); /* read byte from configuration address */
extern uint16_t pci_inw(uint32_t addr, uint32_t offset); /* read word from configuration address */
extern uint32_t pci_ind(uint32_t addr); /* read dword from configuration address */
extern void pci_outb(uint32_t addr, uint8_t b); /* write byte to configuration address */
extern void pci_outw(uint32_t addr, uint16_t w); /* write word to configuration address */
extern void pci_outd(uint32_t addr, uint32_t d); /* write dword to configuration address */
extern uint16_t pci_get_vendor(uint32_t bus, uint32_t dev, uint16_t *devid); /* get vendor for device */
extern void pci_check_function(uint32_t bus, uint32_t dev, uint32_t func); /* check pci device function */
extern void pci_check_device(uint32_t bus, uint32_t dev); /* check pci device */
extern void pci_check_bus(uint32_t bus); /* check pci bus */
extern void pci_register_driver(pci_driver_t *driver); /* register driver */
extern pci_driver_t *pci_match_driver(uint32_t vendor, uint32_t device); /* match device to driver */
extern void pci_init(void); /* initialize pci */

#endif /* ECLAIR_PCI_H */
