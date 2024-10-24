#include <e.clair/types.h>
#include <e.clair/string.h>
#include <e.clair/tty.h>
#include <e.clair/multiboot.h>
#include <e.clair/mm/heap.h>
#include <e.clair/driver/pit.h>
#include <e.clair/driver/rtc.h>
#include <e.clair/driver/ps2.h>
#include <e.clair/driver/ata.h>
#include <e.clair/driver/vgacon.h>
#include <e.clair/driver/device.h>

#define MAX_DEVS 32
static device_t *devs[MAX_DEVS]; /* device pointers */
static int ndevs = 0; /* number of devices */

/* initialize all devices */
extern void device_init(void) {

	vgacon_init_tty();
	pit_init();
	//rtc_init();
	ps2_init();
	ata_init();
}

/* create new device */
extern device_t *device_new(device_type_t type, device_subtype_t subtype, const char *desc, size_t sz) {

	device_t *dev = (device_t *)kmalloc(sz);

	dev->id = ndevs;
	dev->type = type;
	dev->subtype = subtype;
	if (desc) strncpy(dev->desc, desc, DEVICE_DESC_MAX_CHARS);
	else dev->desc[0] = 0;
	devs[ndevs++] = dev;

	return dev;
}

/* debug */
extern void device_print_all(void) {

	for (int i = 0; i < ndevs; i++)
		tty_printf("device: %d, type: %d, subtype: %d, desc: %s\n", devs[i]->id, devs[i]->type, devs[i]->subtype, devs[i]->desc);
}

/* find nth device of type and subtype */
extern device_t *device_find(device_type_t type, device_subtype_t subtype, int n) {

	int f = 0; /* devices found */
	for (int i = 0; i < ndevs; i++) {

		if (devs[i]->type == type && devs[i]->subtype == subtype) {

			if (f < n) f++;
			else return devs[i];
		}
	}
	return NULL; /* none found */
}

/* translate a bios device number */
extern void device_translate_biosdev(uint32_t dev, device_subtype_t *subtp, int *n) {

	if (dev >= 0x80) {
		*subtp = DEVICE_SUBTYPE_STORAGE_ATA;
		*n = dev - 0x80;
	}
	else {
		*subtp = DEVICE_SUBTYPE_NONE;
		*n = 0;
	}
}

/* search for root device */
extern device_t *device_find_root(void) {

	return device_find(DEVICE_TYPE_STORAGE, DEVICE_SUBTYPE_STORAGE_ATA, 0);
}

/* read next int from device */
extern uint32_t device_input_read(device_t *dev, bool block) {

	device_input_t *inpdev = (device_input_t *)dev;

	/* wait for values to become available */
	if (inpdev->s_buf == inpdev->e_buf) {
		if (block) {
			while (inpdev->s_buf == inpdev->e_buf)
				__asm__("hlt");
		}
		else return 0;
	}

	/* get value */
	uint32_t val = inpdev->buf[inpdev->s_buf];
	inpdev->s_buf = (inpdev->s_buf + 1) % DEVICE_INPUT_BUFSZ;
	return val;
}

/* read n blocks from storage device */
extern void device_storage_read(device_t *dev, uint32_t addr, size_t n, void *buf) {

	if (dev->type != DEVICE_TYPE_STORAGE) return;

	device_storage_t *stdev = (device_storage_t *)dev;
	if (stdev->read) stdev->read(dev, addr, n, buf);
}

/* write n blocks to storage device */
extern void device_storage_write(device_t *dev, uint32_t addr, size_t n, void *buf) {

	if (dev->type != DEVICE_TYPE_STORAGE) return;

	device_storage_t *stdev = (device_storage_t *)dev;
	if (stdev->write) stdev->write(dev, addr, n, buf);
}
