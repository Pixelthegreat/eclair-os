#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/pit.h>
#include <kernel/driver/rtc.h>
#include <kernel/driver/ps2.h>
#include <kernel/driver/ata.h>
#include <kernel/driver/vgacon.h>
#include <kernel/driver/fb.h>
#include <kernel/driver/fbcon.h>
#include <kernel/driver/uart.h>
#include <kernel/driver/device.h>

#define MAX_DEVS 32
static device_t *devs[MAX_DEVS]; /* device pointers */
static int ndevs = 0; /* number of devices */

/* initialize tty devices */
static void init_tty(void) {

	if (fb_addr) fbcon_init_tty();
	else vgacon_init_tty();

	uart_init(UART_COM1_BIT, UART_DEFAULT_BAUD_RATE);
	uart_init_tty();
}

/* initialize all devices */
extern void device_init(void) {

	init_tty();
	pit_init();
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
extern uint32_t device_char_read(device_t *dev, bool block) {

	device_char_t *inpdev = (device_char_t *)dev;

	/* wait for values to become available */
	if (inpdev->s_ibuf == inpdev->e_ibuf) {
		if (block) {
			while (inpdev->s_ibuf == inpdev->e_ibuf)
				__asm__("hlt");
		}
		else return 0;
	}

	/* get value */
	uint32_t val = inpdev->ibuf[inpdev->s_ibuf];
	inpdev->s_ibuf = (inpdev->s_ibuf + 1) % DEVICE_CHAR_BUFSZ;
	return val;
}

/* write next int to device */
extern void device_char_write(device_t *dev, uint32_t val, bool flush) {

	device_char_t *outpdev = (device_char_t *)dev;

	outpdev->obuf[outpdev->e_obuf] = val;
	outpdev->e_obuf = (outpdev->e_obuf + 1) % DEVICE_CHAR_BUFSZ;

	/* flush ringbuffer */
	if (flush) outpdev->flush(dev);
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
