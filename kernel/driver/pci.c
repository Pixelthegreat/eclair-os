/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
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

/* read size of a BAR */
extern uint32_t pci_read_bar_size(uint32_t bus, uint32_t dev, uint32_t func, uint32_t i) {

	uint16_t cmd = pci_inw(PCI_ADDR(bus, dev, func, PCI_REG_COMMAND), 0);
	uint16_t newcmd = cmd & ~(PCI_CMD_IO | PCI_CMD_MEM);

	pci_outw(PCI_ADDR(bus, dev, func, PCI_REG_COMMAND), newcmd);

	/* read bar size */
	uint32_t reg = PCI_GENERIC_BAR0+i*4;
	uint32_t orig = pci_ind(PCI_ADDR(bus, dev, func, reg));

	pci_outd(PCI_ADDR(bus, dev, func, reg), 0xffffffff);
	uint32_t res = pci_ind(PCI_ADDR(bus, dev, func, reg));

	pci_outd(PCI_ADDR(bus, dev, func, reg), orig);

	/* restore command and return */
	pci_outw(PCI_ADDR(bus, dev, func, PCI_REG_COMMAND), cmd);
	return res;
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
	
	pci_match_info_t minfo = {vendorid, devid};
	pci_driver_t *driver = NULL;

	uint8_t htype = pci_inb(PCI_ADDR(bus, dev, 0, PCI_REG_HTYPE), 0);
	if (htype & PCI_HTYPE_MFUNC) {
		for (uint32_t i = 0; i < 8; i++) {
			if (pci_inw(PCI_ADDR(bus, dev, i, 0), 0) != 0xffff) {

				pci_check_function(bus, dev, i);
				pdev->func |= (1 << i);

				/* attempt to match driver and initialize device */
				minfo.cls = pci_inb(PCI_ADDR(bus, dev, i, PCI_REG_CLASS), 0);
				minfo.subcls = pci_inb(PCI_ADDR(bus, dev, i, PCI_REG_SUBCLASS), 0);
				minfo.progif = pci_inb(PCI_ADDR(bus, dev, i, PCI_REG_PROGIF), 0);

				driver = pci_match_driver(&minfo);
				if (driver && driver->init) pdev->devs[i] = driver->init(pdev, i, &minfo);
				if (pdev->devs[i]) {

					pdev->drivers[i] = driver;
					kprintf(LOG_INFO, "[pci] Detected device '%s'", pdev->devs[i]->desc);
					device_bus_add(buses[bus]->bus, pdev->devs[i]);
				}
			}
		}
	}
	else {

		pci_check_function(bus, dev, 0);
		pdev->func = 1;

		/* match driver */
		minfo.cls = pci_inb(PCI_ADDR(bus, dev, 0, PCI_REG_CLASS), 0);
		minfo.subcls = pci_inb(PCI_ADDR(bus, dev, 0, PCI_REG_SUBCLASS), 0);
		minfo.progif = pci_inb(PCI_ADDR(bus, dev, 0, PCI_REG_PROGIF), 0);

		driver = pci_match_driver(&minfo);
		if (driver && driver->init) {
			
			pdev->devs[0] = driver->init(pdev, 0, &minfo);
		}
		if (pdev->devs[0]) {

			pdev->drivers[0] = driver;
			kprintf(LOG_INFO, "[pci] Detected device '%s'", pdev->devs[0]->desc);
			device_bus_add(buses[bus]->bus, pdev->devs[0]);
		}
	}
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
extern pci_driver_t *pci_match_driver(pci_match_info_t *info) {

	pci_driver_t *cur = dfirst;
	while (cur) {

		if (cur->match && cur->match(info))
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

/* initialize pci vfs nodes */
extern void pci_init_devfs(void) {

	for (int i = 0; i < 256; i++) {

		pci_bus_t *bus = buses[i];
		if (!bus) continue;

		for (int j = 0; j < 32; j++) {

			pci_device_t *pdev = &bus->devs[j];
			if (!pdev) continue;

			for (int k = 0; k < 8; k++) {

				device_t *dev = pdev->devs[k];
				pci_driver_t *driver = pdev->drivers[k];
				if (!dev || !driver) continue;

				if (driver->init_devfs) driver->init_devfs(dev);
			}
		}
	}
}
