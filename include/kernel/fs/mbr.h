/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_FS_MBR_H
#define ECLAIR_FS_MBR_H

#include <kernel/types.h>
#include <kernel/driver/device.h>
#include <kernel/vfs/fs.h>

/* mbr entry */
typedef struct mbr_ent {
	uint8_t attr; /* drive attributes */
	uint8_t start_chs[3]; /* chs address of start */
	uint8_t type; /* partition type */
	uint8_t end_chs[3]; /* chs address of end */
	uint32_t start_lba; /* lba address of start */
	uint32_t nsects; /* number of sectors */
} __attribute__((packed)) mbr_ent_t;

/* mbr/partition table */
typedef struct mbr {
	uint8_t pad[440]; /* executable code */
	uint32_t diskid; /* disk id/signature */
	uint16_t rsvd; /* reserved */
	mbr_ent_t ents[4]; /* partition table entries */
	uint16_t bootsig; /* boot signature */
} __attribute__((packed)) mbr_t;

/* mbr file system identifiers */
#define MBR_FS_LINUX 0x83
#define MBR_FS_ECFS 0xec

/* mbr fs driver info */
typedef struct mbr_driver {
	uint8_t fstype; /* file system type/identifier */
	fs_node_t *(*mount)(fs_node_t *, device_t *, mbr_ent_t *); /* mount file system */
} mbr_driver_t;

/* functions */
extern mbr_t *mbr_get_table(device_t *dev); /* get mbr table from device */
extern void mbr_print(mbr_t *mbr); /* print info from mbr */
extern fs_node_t *mbr_fs_mount(fs_node_t *node, device_t *dev, mbr_ent_t *ent); /* try to mount mbr partition */
extern fs_node_t *mbr_fs_probe(device_t *dev, mbr_t *mbr); /* probe mbr for file systems */
extern void mbr_fs_mount_root(void); /* search for root filesystem */

#endif /* ECLAIR_FS_MBR_H */
