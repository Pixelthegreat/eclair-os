#include <e.clair/types.h>
#include <e.clair/tty.h>
#include <e.clair/mm/heap.h>
#include <e.clair/driver/device.h>
#include <e.clair/fs/ext2.h>
#include <e.clair/fs/mbr.h>

/* get mbr table from device */
extern mbr_t *mbr_get_table(device_t *dev) {

	uint8_t *buf = (uint8_t *)kmalloc(512);
	device_storage_read(dev, 0, 1, buf);

	mbr_t *mbr = (mbr_t *)buf;

	/* invalid boot signature */
	if (mbr->bootsig != 0xaa55) {

		kfree(buf);
		return NULL;
	}
	return mbr;
}

/* print info from mbr */
extern void mbr_print(mbr_t *mbr) {

	tty_printf("disk id: 0x%x\nboot signature: 0x%x\n", mbr->diskid, mbr->bootsig);
	for (int i = 0; i < 4; i++) {

		if (!(mbr->ents[i].attr && 0x80)) continue;

		tty_printf("  partition: %d\n  drive attributes: 0x%x\n  type: 0x%x\n  lba address of start sector: 0x%x\n  number of sectors: 0x%x\n", i, mbr->ents[i].attr, mbr->ents[i].type, mbr->ents[i].start_lba, mbr->ents[i].nsects);
	}
}

/* try to mount mbr partition */
extern fs_node_t *mbr_fs_mount(fs_node_t *node, device_t *dev, mbr_ent_t *ent) {

	if (ent->type == MBR_FS_LINUX)
		return ext2_mbr_mount(node, dev, ent);
	return NULL;
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
