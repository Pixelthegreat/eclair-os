/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/boot.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/device.h>
#include <kernel/fs/tarfs.h>
#include <kernel/fs/mbr.h>

/* bootloader id info */
struct boot_id {
	uint32_t magic;
	char osid[12];
};
static char osid[12] = "eclair-os   ";

static void *mbrbuf = NULL;

/* get mbr table from device */
extern mbr_t *mbr_get_table(device_t *dev) {

	if (!mbrbuf) mbrbuf = kmalloc(512);
	device_storage_read(dev, 0, 1, mbrbuf);

	mbr_t *mbr = (mbr_t *)mbrbuf;

	/* invalid boot signature */
	if (mbr->bootsig != 0xaa55)
		return NULL;
	return mbr;
}

/* print info from mbr */
extern void mbr_print(mbr_t *mbr) {

	kprintf(LOG_INFO, "Disk ID: 0x%x; Boot signature: 0x%x", mbr->diskid, mbr->bootsig);
	for (int i = 0; i < 4; i++) {

		if (!(mbr->ents[i].attr && 0x80)) continue;

		kprintf(LOG_INFO, "  Partition: %d; Drive attributes: 0x%x; Type: 0x%x; LBA address of start sector: 0x%x; Number of sectors: 0x%x", i, mbr->ents[i].attr, mbr->ents[i].type, mbr->ents[i].start_lba, mbr->ents[i].nsects);
	}
}

/* try to mount mbr partition */
extern fs_node_t *mbr_fs_mount(fs_node_t *node, device_t *dev, mbr_ent_t *ent) {

	fs_node_t *res = NULL;
	const char *name = NULL;

	driverinfo_t *drivers = DRIVERINFO_START;
	size_t count = DRIVERINFO_COUNT;

	/* search for fs driver */
	for (size_t i = 0; i < count; i++) {
		if (drivers[i].type == DRIVERINFO_MBRFS) {

			mbr_driver_t *md = (mbr_driver_t *)drivers[i].data;
			if (ent->type == md->fstype) {

				res = md->mount(node, dev, ent);
				name = drivers[i].name;
				break;
			}
		}
	}

	if (res) {

		if (!name) kprintf(LOG_INFO, "[mbr] Mounted filesystem on device '%s'", dev->desc);
		else kprintf(LOG_INFO, "[mbr] Mounted filesystem on device '%s' as %s", dev->desc, name);
	}
	return res;
}

/* probe mbr for file systems */
extern fs_node_t *mbr_fs_probe(device_t *dev, mbr_t *mbr) {

	for (int i = 0; i < 4; i++) {

		fs_node_t *node;
		if ((mbr->ents[i].attr & 0x80) && (node = mbr_fs_mount(fs_root, dev, &mbr->ents[i])))
			return node;
	}
	return NULL;
}

/* search for root filesystem */
extern void mbr_fs_mount_root(void) {

	fs_node_t *node = NULL;
	boot_saved_info_t *info = boot_get_saved_info();
	if (info->initrd_addr) {

		if ((node = tarfs_mount(fs_root, info->initrd_addr)))
			kprintf(LOG_INFO, "[mbr] Mounted root filesystem as tarfs ramdisk");
	}

	device_t *dev = devclass_storage.first;
	int i = 0;
	
	for (; dev && !node; dev = dev->clsnext) {

		/* get partition info */
		mbr_t *mbr = mbr_get_table(dev);
		if (!mbr) continue;

		struct boot_id *id = (struct boot_id *)((void *)mbr+16);
		if (id->magic != BOOT_MAGIC || !!memcmp(id->osid, osid, 12))
			continue;

		node = mbr_fs_probe(dev, mbr);
	}

	if (!node) kpanic(PANIC_CODE_NONE, "Failed to mount root file system\n", NULL);
}
