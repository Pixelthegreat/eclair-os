/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef EC_MBR_H
#define EC_MBR_H

#include <stdint.h>

#define EC_MBR_START 440

/* mbr partition */
#define EC_MBR_ATTR_BOOT 0x80

typedef struct ec_mbr_part {
	uint8_t attr; /* drive attributes */
	uint8_t start_chs[3]; /* chs address of partition start */
	uint8_t type; /* type of partition */
	uint8_t last_chs[3]; /* chs address of last partition sector */
	uint32_t start_lba; /* lba address of partition start */
	uint32_t nsect; /* number of partition sectors */
} __attribute__((packed)) ec_mbr_part_t;

/* mbr structure */
typedef struct ec_mbr {
	uint8_t diskid[4]; /* disk identifier */
	uint16_t rsvd; /* reserved */
	ec_mbr_part_t part[4]; /* partitions */
	uint8_t bootsig[2]; /* boot signature */
} __attribute__((packed)) ec_mbr_t;

#endif /* EC_MBR_H */
