#include <e.clair/types.h>
#include <e.clair/tty.h>
#include <e.clair/mm/heap.h>
#include <e.clair/io/port.h>
#include <e.clair/driver/device.h>
#include <e.clair/driver/ata.h>

static device_t *devs[4]; /* ata devices */
static const char *dev_names[4] = {
	"ATA Primary Master",
	"ATA Primary Slave",
	"ATA Secondary Master",
	"ATA Secondary Slave",
};

static uint8_t status; /* status register */

/* wait for controller to be ready */
#define ATA_WAIT_MAX 4096

static int ata_wait_flag(uint16_t port, uint8_t flags) {

	for (int i = 0; i < ATA_WAIT_MAX; i++) {
		if (!((status = port_inb(port + ATA_PORT_COMMAND)) & flags))
			return 0;
	}
	return -1; /* timeout */
}

/* use a device number on a controller */
static void ata_use_dev(int c, int d) {

	uint16_t p = c? ATA_PORT_SECONDARY: ATA_PORT_PRIMARY;
	if (ata_wait_flag(p, ATA_STATUS_BSY | ATA_STATUS_DRQ) < 0) return;

	port_outb(p + ATA_PORT_DEVICE, 0x40 | (d << 4));
}

/* read n words */
static void ata_read_words(uint16_t port, uint16_t *buf, int cnt) {

	for (int i = 0; i < cnt * 2; i++)
		buf[i] = port_inb(port + ATA_PORT_DATA);
}

/* detect device */
static device_t *ata_detect_device(int c, int d) {

	uint16_t port = c? ATA_PORT_SECONDARY: ATA_PORT_PRIMARY;

	/* send initial command request */
	ata_use_dev(c, d);
	port_outb(port + ATA_PORT_COMMAND, ATA_COMMAND_IDENTIFY);

	/* no device */
	if (ata_wait_flag(port, ATA_STATUS_BSY) < 0 || !(status & ATA_STATUS_DRQ))
		return NULL;

	/* use data */
	uint16_t *buf = (uint16_t *)kmalloc(512);
	ata_read_words(port, buf, 256);

	/* (todo) */
	
	kfree(buf);

	return device_new(DEVICE_TYPE_STORAGE, DEVICE_SUBTYPE_STORAGE_ATA, dev_names[c * 2 + d], sizeof(device_storage_t));
}

/* initialize */
extern void ata_init(void) {

	devs[0] = ata_detect_device(0, 0);
	devs[1] = ata_detect_device(0, 1);
	devs[2] = ata_detect_device(1, 0);
	devs[3] = ata_detect_device(1, 1);
}
