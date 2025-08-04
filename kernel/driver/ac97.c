/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/io/port.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/device.h>
#include <kernel/driver/pit.h>
#include <kernel/driver/pci.h>
#include <kernel/vfs/fs.h>
#include <kernel/vfs/devfs.h>
#include <ec/device.h>
#include <kernel/driver/ac97.h>

/* ac97 device */
struct ac97_device {
	pci_device_t *pdev; /* pci device */
	uint32_t func; /* device function */
	uint16_t nam; /* native audio mixer base io address */
	uint16_t nabm; /* native audio bus master base io address */
	uint32_t irq; /* pic irq number */
	ac97_bdl_entry_t *bdl; /* buffer descriptor list */
	void *buffers[32]; /* buffer data */
	uint32_t pos; /* ringbuffer position */
	uint32_t nwritten; /* number of buffers written */
	bool started; /* data transfer started */
	void *physmem; /* bdl physical memory base */
} *device = NULL; /* main device */

/* driver info */
static bool ac97_match(pci_match_info_t *info);
static device_t *ac97_init(pci_device_t *pdev, uint32_t func, pci_match_info_t *info);
static fs_node_t *ac97_init_devfs(device_t *_device);

static pci_driver_t driver = {
	.match = ac97_match,
	.init = ac97_init,
	.init_devfs = ac97_init_devfs,
};
DRIVERINFO(ac97, DRIVERINFO_PCI, &driver);

/* match with pci device */
static bool ac97_match(pci_match_info_t *info) {

	return (info->vendor == AC97_VENDORID &&
	        info->device == AC97_DEVICEID);
}

/* ac97 irq callback */
static void ac97_irq(idt_regs_t *regs) {

	device->pos++;
	if (device->pos >= 31) {

		device->pos = 0;
		if (device->nwritten)
			port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL, AC97_TSTAT_IOC | AC97_TSTAT_DMA);
	}
	if (device->nwritten) device->nwritten--;

	port_outw(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TSTAT, 0x1c);
}

/* create kernel device */
static device_t *ac97_init(pci_device_t *pdev, uint32_t func, pci_match_info_t *info) {

	if (device) return NULL;

	device = (struct ac97_device *)kmalloc(sizeof(struct ac97_device));
	device->pdev = pdev;
	device->func = func;

	device->nam = (uint16_t)pci_ind(PCI_ADDR(pdev->nbus, pdev->ndev, func, PCI_GENERIC_BAR0)) & 0xff00;
	device->nabm = (uint16_t)pci_ind(PCI_ADDR(pdev->nbus, pdev->ndev, func, PCI_GENERIC_BAR1)) & 0xffc0;
	device->irq = (uint32_t)pci_inb(PCI_ADDR(pdev->nbus, pdev->ndev, func, PCI_GENERIC_INTLINE), 0);
	device->pos = 0;
	device->nwritten = 0;
	device->started = false;

	idt_set_irq_callback(device->irq, ac97_irq);

	/* initialize device */
	pci_outw(PCI_ADDR(pdev->nbus, pdev->ndev, func, PCI_REG_COMMAND), PCI_CMD_IO | PCI_CMD_BM);

	port_outd(device->nabm+AC97_NABM_GCR, AC97_GCR_CRESET | AC97_GCR_IE);
	pit_delay_ms(20);

	port_outw(device->nam+AC97_NAM_RESET, 0xffff);
	port_outw(device->nam+AC97_NAM_PCM_VOLUME, 0);

	/* create buffer descriptor list */
	void *mem = kmalloca(0x1000 * 33, 0x1000);
	memset(mem, 0, 0x1000 * 33);

	void *physmem = (void *)(page_get_frame((uint32_t)mem >> 12) << 12);
	device->physmem = physmem;

	device->bdl = (ac97_bdl_entry_t *)mem;
	for (uint32_t i = 0; i < 32; i++) {

		device->bdl[i].addr = (uint32_t)physmem + 0x1000 * (i + 1);
		device->bdl[i].ctrl = 0x800 | AC97_BDL_STATUS_INT;
		device->buffers[i] = mem + 0x1000 * (i + 1);
	}
	port_outw(device->nam+AC97_NAM_MASTER_VOLUME, 0);

	/* test start */
	port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL, AC97_TSTAT_EOT);
	while (port_inb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL) & AC97_TSTAT_EOT);

	port_outd(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_BDL, (uint32_t)device->physmem);

	port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_NDESC, 2);
	port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL, AC97_TSTAT_IOC | AC97_TSTAT_DMA);

	return device_audio_new("AC97 Audio Controller");
}

/* write data */
static kssize_t write_fs(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buf) {

	if (size != 0x1000) return -EINVAL;
	if (!device->started) return -EPERM;
	if (device->nwritten >= 30) return 0;

	uint32_t pos = (device->pos + (++device->nwritten)) % 31;
	memcpy(device->buffers[pos], buf, size);

	return (kssize_t)size;
}

/* io control */
static int ioctl_fs(fs_node_t *node, int op, uintptr_t arg) {

	switch (op) {
		/* get/set attributes */
		case ECIO_SND_SETATTR:
			ecio_sndattr_t *attr = (ecio_sndattr_t *)arg;
			if (!attr) return -EINVAL;

			attr->channels = 2;
			attr->rate = 48000;
			attr->format = ECIO_SNDFMT_I16;
			attr->bufsz = 0x1000;
			return 0;
		/* start playback */
		case ECIO_SND_START:
			if (device->started) return -EPERM;

			device->pos = 0;
			device->nwritten = 0;
			device->started = true;
			memset(device->buffers[0], 0, 0x1000);

			/* reset box */
			port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL, AC97_TSTAT_EOT);
			while (port_inb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL) & AC97_TSTAT_EOT);

			port_outd(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_BDL, (uint32_t)device->physmem);

			port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_NDESC, 31);
			port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL, AC97_TSTAT_IOC | AC97_TSTAT_DMA);
			return 0;
		/* stop playback */
		case ECIO_SND_STOP:
			if (!device->started) return -EPERM;

			port_outb(device->nabm+AC97_NABM_PCM_OUT+AC97_NABM_BOX_TCTRL, 0);
			device->started = false;
			return 0;
		default:
			return -ENOSYS;
	}
}

/* add node to device filesystem */
static fs_node_t *ac97_init_devfs(device_t *_device) {

	fs_node_t *node = fs_node_new(NULL, FS_CHARDEVICE);

	node->mask = 0666;
	node->write = write_fs;
	node->ioctl = ioctl_fs;

	devfs_add_node("snd", node);
	return node;
}

/* register pci driver */
extern void ac97_register(void) {

	pci_register_driver(&driver);
}
