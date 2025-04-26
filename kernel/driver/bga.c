#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/driver/device.h>
#include <kernel/driver/pci.h>
#include <kernel/driver/bga.h>

static pci_device_t *pdev = NULL;

/* driver info */
static bool bga_match(pci_match_info_t *info);
static device_t *bga_init(pci_device_t *_pdev, uint32_t func, pci_match_info_t *info);

static pci_driver_t driver = {
	.match = bga_match,
	.init = bga_init,
};
DRIVERINFO(bga, DRIVERINFO_PCI, &driver);

/* match with pci device */
static bool bga_match(pci_match_info_t *info) {

	return (info->vendor == BGA_VENDORID) &&\
	       (info->device == BGA_DEVICEID);
}

/* create kernel device */
static device_t *bga_init(pci_device_t *_pdev, uint32_t func, pci_match_info_t *info) {

	if (pdev) return NULL;
	pdev = _pdev;

	return device_video_new("Bochs Graphics Adapter");
}

/* register pci driver */
extern void bga_register(void) {

	pci_register_driver(&driver);
}
