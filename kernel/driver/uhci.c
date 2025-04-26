/*
 * NOTE: This UHCI driver is not intended to be a fully
 * featured UHCI driver. I will probably not implement
 * a full USB stack ever; However, I do wish to be able
 * to roll out basic support for USB mass storage
 * devices, hence this driver.
 */
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/io/port.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/device.h>
#include <kernel/driver/pit.h>
#include <kernel/driver/pci.h>
#include <kernel/driver/uhci.h>

#define MAX_DEVS 8
static uhci_info_t devs[MAX_DEVS];
static int ndevs = 0;

/* driver info */
static bool uhci_match(pci_match_info_t *info);
static device_t *uhci_init(pci_device_t *pdev, uint32_t func, pci_match_info_t *info);

static pci_driver_t driver = {
	.match = uhci_match,
	.init = uhci_init,
};
DRIVERINFO(uhci, DRIVERINFO_PCI, &driver);

/* match with pci device */
static bool uhci_match(pci_match_info_t *info) {

	return (info->cls == UHCI_CLASS) &&
	       (info->subcls == UHCI_SUBCLASS) &&
	       (info->progif == UHCI_PROGIF);
}

/* create kernel device */
static device_t *uhci_init(pci_device_t *pdev, uint32_t func, pci_match_info_t *info) {

	if (ndevs >= MAX_DEVS) return NULL;

	uint32_t bar4 = pci_ind(PCI_ADDR(pdev->nbus, pdev->ndev, func, PCI_GENERIC_BAR4));
	uint32_t addr = bar4 & 0xfffffffc;

	uint32_t bar4sz = pci_read_bar_size(pdev->nbus, pdev->ndev, func, 4);
	uint32_t size = ~(bar4sz & 0xfffffffc) + 1;

	/* disable legacy usb emulation and enable io bus mastering */
	uint16_t legsup = pci_inw(PCI_ADDR(pdev->nbus, pdev->ndev, func, UHCI_PCIREG_LEGSUP), 0);

	legsup &= ~(UHCI_LEGSUP_PIRQ);
	pci_outw(PCI_ADDR(pdev->nbus, pdev->ndev, func, UHCI_PCIREG_LEGSUP), legsup);

	uint16_t cmd = pci_inw(PCI_ADDR(pdev->nbus, pdev->ndev, func, PCI_REG_COMMAND), 0);

	cmd = cmd | PCI_CMD_IO | PCI_CMD_BM;
	pci_outw(PCI_ADDR(pdev->nbus, pdev->ndev, func, PCI_REG_COMMAND), cmd);

	/* create device */
	device_t *dev = device_bus_new("USB UHCI");

	uhci_info_t *uinfo = &devs[ndevs];
	uinfo->iobase = (uint16_t)addr;
	uinfo->iosize = (uint16_t)size;
	dev->impl = ndevs++;

	/* reset host controller */
	uint16_t usbcmd = uhci_inw(dev, UHCI_REG_USBCMD);
	usbcmd |= UHCI_USBCMD_HCRESET;
	uhci_outw(dev, UHCI_REG_USBCMD, usbcmd);

	while ((usbcmd = uhci_inw(dev, UHCI_REG_USBCMD)) & UHCI_USBCMD_HCRESET)
		pit_delay_ms(10);

	usbcmd |= UHCI_USBCMD_GRESET;
	uhci_outw(dev, UHCI_REG_USBCMD, usbcmd);

	pit_delay_ms(10);

	usbcmd &= ~(UHCI_USBCMD_GRESET);
	uhci_outw(dev, UHCI_REG_USBCMD, usbcmd);

	/* allocate frame list */
	uinfo->flist = (uint32_t *)kmalloca(4096, 4096);
	for (uint32_t i = 0; i < 1024; i++)
		uinfo->flist[i] = UHCI_FRAME_EMPTY;
	uint32_t paddr = page_get_frame((uint32_t)uinfo->flist >> 12) << 12;

	uhci_outd(dev, UHCI_REG_FRBASEADD, paddr);

	/* enable interrupts */
	uint16_t intr = uhci_inw(dev, UHCI_REG_USBINTR);
	intr = intr |
	       UHCI_USBINTR_TMCRC |
	       UHCI_USBINTR_RESUME |
	       UHCI_USBINTR_COMPTFR |
	       UHCI_USBINTR_SHORTPK;
	uhci_outw(dev, UHCI_REG_USBINTR, intr);

	/* enumerate ports */
	uint16_t max = uinfo->iosize - UHCI_REG_PORTSC1;
	for (uint16_t i = 0; i < max; i += 2) {

		uint16_t port = uhci_inw(dev, UHCI_REG_PORTSC1+i);
		if (port & UHCI_PORT_PRESENT && port != 0xffff) {

			/* ... */
		}
	}

	/* enable controller */
	usbcmd = usbcmd | UHCI_USBCMD_RUN | UHCI_USBCMD_MAXPKSZ;
	uhci_outw(dev, UHCI_REG_USBCMD, usbcmd);

	return dev;
}

/* read word from uhci port */
extern uint16_t uhci_inw(device_t *dev, uint16_t port) {

	return port_inw(devs[dev->impl].iobase + port);
}

/* write word to uhci port */
extern void uhci_outw(device_t *dev, uint16_t port, uint16_t data) {

	port_outw(devs[dev->impl].iobase + port, data);
}

/* read dword from uhci port */
extern uint32_t uhci_ind(device_t *dev, uint16_t port) {

	return port_ind(devs[dev->impl].iobase + port);
}

/* write dword to uhci port */
extern void uhci_outd(device_t *dev, uint16_t port, uint32_t data) {

	port_outw(devs[dev->impl].iobase + port, data);
}

/* register pci driver */
extern void uhci_register(void) {

	pci_register_driver(&driver);
}
