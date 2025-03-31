#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/io/port.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/pci.h>

static pci_driver_t *dfirst = NULL; /* first pci driver */
static pci_driver_t *dlast = NULL; /* last driver */

static pci_bus_t *buses[256];

/* read byte from configuration address */
extern uint8_t pci_inb(uint32_t addr, uint32_t offset) {

	port_outd(PCI_PORT_ADDR, addr);
	return (uint8_t)((port_ind(PCI_PORT_DATA) >> offset) & 0xff);
}

/* read word from configuration address */
extern uint16_t pci_inw(uint32_t addr, uint32_t offset) {

	port_outd(PCI_PORT_ADDR, addr);
	return (uint16_t)((port_ind(PCI_PORT_DATA) >> offset) & 0xffff);
}

/* read dword from configuration address */
extern uint32_t pci_ind(uint32_t addr) {

	port_outd(PCI_PORT_ADDR, addr);
	return port_ind(PCI_PORT_DATA);
}

/* write byte to configuration address */
extern void pci_outb(uint32_t addr, uint8_t b) {

	port_outd(PCI_PORT_ADDR, addr);
	port_outd(PCI_PORT_DATA, (uint32_t)b);
}

/* write word to configuration address */
extern void pci_outw(uint32_t addr, uint16_t w) {

	port_outd(PCI_PORT_ADDR, addr);
	port_outd(PCI_PORT_DATA, (uint32_t)w);
}

/* write dword to configuration address */
extern void pci_outd(uint32_t addr, uint32_t d) {

	port_outd(PCI_PORT_ADDR, addr);
	port_outd(PCI_PORT_DATA, d);
}

/* get vendor for device */
extern uint16_t pci_get_vendor(uint32_t bus, uint32_t dev, uint16_t *devid) {

	uint32_t data = pci_ind(PCI_ADDR(bus, dev, 0, 0));
	if (devid) *devid = (uint16_t)((data >> 16) & 0xffff);

	return (uint16_t)(data & 0xffff);
}

/* check pci device function */
extern void pci_check_function(uint32_t bus, uint32_t dev, uint32_t func) {

	uint8_t cls = pci_inb(PCI_ADDR(bus, dev, func, PCI_REG_CLASS), 0);
	uint8_t subcls = pci_inb(PCI_ADDR(bus, dev, func, PCI_REG_SUBCLASS), 0);

	if (cls == PCI_CLASS_BRIDGE && subcls == PCI_SUBCLASS_PCI2PCI_BRIDGE)
		pci_check_bus((uint32_t)pci_inb(PCI_ADDR(bus, dev, func, PCI_PCI2PCI_NBUS2), 0));
}

/* check pci device */
extern void pci_check_device(uint32_t bus, uint32_t dev) {

	uint16_t vendorid, devid;
	vendorid = pci_get_vendor(bus, dev, &devid);
	if (vendorid == 0xffff) return;

	pci_device_t *pdev = &buses[bus]->devs[dev];
	pdev->present = true;
	pdev->nbus = bus;
	pdev->ndev = dev;

	uint8_t htype = pci_inb(PCI_ADDR(bus, dev, 0, PCI_REG_HTYPE), 0);
	if (htype & PCI_HTYPE_MFUNC) {
		for (uint32_t i = 0; i < 8; i++) {
			if (pci_inw(PCI_ADDR(bus, dev, i, 0), 0) != 0xffff) {

				pci_check_function(bus, dev, i);
				pdev->func |= (1 << i);
			}
		}
	}
	else {

		pci_check_function(bus, dev, 0);
		pdev->func = 1;
	}

	/* match device and initialize driver */
	pci_driver_t *driver = pci_match_driver(vendorid, devid);
	if (driver && driver->init) pdev->device = driver->init(pdev);
}

/* check pci bus */
extern void pci_check_bus(uint32_t bus) {

	device_t *dev = device_bus_new("PCI Bus");

	buses[bus] = kmalloc(sizeof(pci_bus_t));
	memset(buses[bus], 0, sizeof(pci_bus_t));

	buses[bus]->nbus = bus;
	buses[bus]->bus = dev;

	for (uint32_t i = 0; i < 32; i++)
		pci_check_device(bus, i);
}

/* register driver */
extern void pci_register_driver(pci_driver_t *driver) {

	if (!dfirst) dfirst = driver;
	if (dlast) dlast->next = driver;
	dlast = driver;
	driver->next = NULL;
}

/* match device to driver */
extern pci_driver_t *pci_match_driver(uint32_t vendor, uint32_t device) {

	pci_driver_t *cur = dfirst;
	while (cur) {

		if (cur->match && cur->match(vendor, device))
			break;
		cur = cur->next;
	}
	return cur;
}

/* initialize pci */
extern void pci_init(void) {

	uint8_t htype = pci_inb(PCI_ADDR(0, 0, 0, PCI_REG_HTYPE), 0);

	if (htype & PCI_HTYPE_MFUNC) {
		for (uint32_t i = 0; i < 8; i++) {
			if (pci_inw(PCI_ADDR(0, 0, i, 0), 0) != 0xffff)
				pci_check_bus(i);
		}
	}
	else pci_check_bus(0);
}
