#include <kernel/types.h>
#include <kernel/mm/heap.h>
#include <kernel/io/port.h>
#include <kernel/driver/device.h>
#include <kernel/driver/ata.h>

#define ATA_WAIT_MAX 4096

static device_t *devs[4]; /* ata devices */
static const char *dev_names[4] = {
	"ATA Primary Master",
	"ATA Primary Slave",
	"ATA Secondary Master",
	"ATA Secondary Slave",
};

static uint8_t status; /* status register */

/* wait for controller to be ready */
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

	for (int i = 0; i < cnt; i++)
		buf[i] = port_inw(port + ATA_PORT_DATA);
}

/* write n words */
static void ata_write_words(uint16_t port, uint16_t *buf, int cnt) {

	for (int i = 0; i < cnt; i++)
		port_outw(port + ATA_PORT_DATA, buf[i]);
}

/* read lba */
static void ata_read_lba(device_t *dev, uint32_t addr, size_t n, void *buf) {

	int c = dev->impl / 2;
	int d = dev->impl % 2;
	uint16_t port = c? ATA_PORT_SECONDARY: ATA_PORT_PRIMARY;
	uint8_t cmd = (n == 1)? ATA_COMMAND_READ_SECTORS: ATA_COMMAND_READ_MULTIPLE;

	ata_use_dev(c, d);

	/* write command parameters */
	port_outb(port + ATA_PORT_SECTOR_COUNT, n);
	port_outb(port + ATA_PORT_LBA_LOW, addr & 0xff);
	port_outb(port + ATA_PORT_LBA_MID, (addr >> 8) & 0xff);
	port_outb(port + ATA_PORT_LBA_HIGH, (addr >> 16) & 0xff);
	port_outb(port + ATA_PORT_DEVICE, (0x40 | (d << 4)) | ((addr >> 24) & 0xf));
	
	/* send command */
	port_outb(port + ATA_PORT_COMMAND, cmd);
	if (ata_wait_flag(port, ATA_STATUS_BSY) < 0 || !(status & ATA_STATUS_DRQ))
		return;

	ata_read_words(port, buf, n * 256);
}

/* write lba */
static void ata_write_lba(device_t *dev, uint32_t addr, size_t n, void *buf) {

	int c = dev->impl / 2;
	int d = dev->impl % 2;
	uint16_t port = c? ATA_PORT_SECONDARY: ATA_PORT_PRIMARY;
	uint8_t cmd = (n == 1)? ATA_COMMAND_WRITE_SECTORS: ATA_COMMAND_WRITE_MULTIPLE;

	ata_use_dev(c, d);

	/* write command parameters */
	port_outb(port + ATA_PORT_SECTOR_COUNT, n);
	port_outb(port + ATA_PORT_LBA_LOW, addr & 0xff);
	port_outb(port + ATA_PORT_LBA_MID, (addr >> 8) & 0xff);
	port_outb(port + ATA_PORT_LBA_HIGH, (addr >> 16) & 0xff);
	port_outb(port + ATA_PORT_DEVICE, (0x40 | (d << 4)) | ((addr >> 24) & 0xf));

	/* send command */
	port_outb(port + ATA_PORT_COMMAND, cmd);
	if (ata_wait_flag(port, ATA_STATUS_BSY) < 0 || !(status & ATA_STATUS_DRQ))
		return;
	
	ata_write_words(port, buf, n * 256);
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
	
	kfree(buf);

	device_t *dev = device_new(DEVICE_TYPE_STORAGE, DEVICE_SUBTYPE_STORAGE_ATA, dev_names[c * 2 + d], sizeof(device_storage_t));
	device_storage_t *stdev = (device_storage_t *)dev;
	
	dev->impl = c * 2 + d; /* device index number */
	stdev->read = ata_read_lba;
	stdev->write = ata_write_lba;

	return dev;
}

/* initialize */
extern void ata_init(void) {

	devs[0] = ata_detect_device(0, 0);
	devs[1] = ata_detect_device(0, 1);
	devs[2] = ata_detect_device(1, 0);
	devs[3] = ata_detect_device(1, 1);
}
