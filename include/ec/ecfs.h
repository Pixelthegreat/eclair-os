/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Eclair OS File System Structure Definitions
 */
#ifndef ECFS_H
#define ECFS_H

#include <stdint.h>

#define ECFS_REVISION 1

#define ECFS_PACKED __attribute__((packed))
#define ECFS_ALIGN(x, sz) (((x) + ((sz)-1)) & ~((sz)-1))

/* Core Notes
 *   All data is little endian. This file currently
 *   describes revision 1.
 */

/*
 * Head Block: 1024 bytes, starting at byte 1024 of the volume
 *   Contains important metadata about the file system,
 *   as shown below.
 *
 *   For practicality reasons, it is not recommended to
 *   use block sizes larger than 65536. However
 *
 *   The (hblk.osid) field does not necessarily have to
 *   be any specific kind of data, but it is
 *   recommended to be an ASCII or UTF-8 string padded
 *   with spaces.
 *
 *   The example for Eclair OS is: 'eclair-os   '
 */
#define ECFS_HBLK_START 1024
#define ECFS_HBLK_SIZE 1024

#define ECFS_SIG {'\x2f', '\xec', 'E', 'C'}

typedef struct ecfs_hblk {
	char signature[4]; /* should be 0x2f, 0xec, 'E', 'C' */
	char osid[12]; /* operating system identifier */
	uint32_t blk_tlrm; /* block number of the toplevel reservation map */
	uint32_t nblk_tlrm; /* number of blocks consumed by the TLRM */
	uint32_t blk_brb; /* block number of the block reservation bitmap */
	uint32_t nblk_brb; /* number of blocks consumed by the BRB */
	uint32_t blk_root; /* block number of the root file entry */
	uint32_t blksz; /* block size; must be power of 2 and greater than or equal to 1024 */
	uint32_t nblk; /* number of blocks in volume */
	uint32_t nblk_alloc; /* number of allocated blocks in volume */
	uint32_t nblk_rsvd; /* number of reserved blocks in the beginning of the volume */
	uint32_t revision; /* file system revision number */
} ECFS_PACKED ecfs_hblk_t;

/* Toplevel Reservation Map (TLRM)
 *   Contains a packed array of uint32_t values that
 *   each indicate the number of allocated blocks in
 *   the respective block area. The size of a block
 *   area is (hblk.blksz * 8). Block areas are laid out
 *   in a linear fashion, similar to blocks.
 *
 *   The (hblk.nblk_rsvd) field includes this map.
 */

/* Block Reservation Bitmap (BRB)
 *   Contains a bitmap in which each bit indicates
 *   whether the associated block is allocated (1) or
 *   free (0). Each block is laid out in a linear
 *   fashion. Each block in the BRB can represent
 *   (hblk.blksz * 8) blocks in the volume, and the
 *   area that it represents is an aforementioned block
 *   area.
 *
 *   The (hblk.nblk_rsvd) field also includes this
 *   bitmap.
 */

/* File Entry
 *   Contains information about a file. A file entry is
 *   always exactly one block in size.
 *
 *   Following the main structure of a file entry is an
 *   array of uint32_t block indices that point to the
 *   data blocks of the file. The final block index (if
 *   present) points to an extension block, which is
 *   another array of uint32_t block indices. The final
 *   block index of an extension block can point to
 *   another extension block, and so on.
 *
 *   If the file is a directory, then the block indices
 *   point to file entries. In this case, a block index
 *   of zero should be ignored and NOT be seen as a
 *   sign of an invalid or corrupted filesystem.
 *   Additionally, the (file.nblk) should include any
 *   extension blocks (directories only).
 *
 *   If the file is a symbolic link, then the contents
 *   of the file indicate a link path.
 *
 *   The (hblk.nblk_rsvd) field includes the root file
 *   entry.
 */
#define ECFS_FLAG_REG 0x1 /* regular file */
#define ECFS_FLAG_DIR 0x2 /* directory */
#define ECFS_FLAG_SLINK 0x4 /* symbolic link */

/* permission masks just taken from posix for driver simplicity */
#define ECFS_MASK_OX 01 /* other execute */
#define ECFS_MASK_OW 02 /* other write */
#define ECFS_MASK_OR 04 /* other read */

#define ECFS_MASK_GX 010 /* group execute */
#define ECFS_MASK_GW 020 /* group write */
#define ECFS_MASK_GR 040 /* group read */

#define ECFS_MASK_UX 0100 /* user execute */
#define ECFS_MASK_UW 0200 /* user write */
#define ECFS_MASK_UR 0400 /* user read */

typedef struct ecfs_file {
	char name[128]; /* null-terminated UTF-8 filename string */
	uint32_t flags; /* file flags (see above) */
	uint32_t uid; /* user id */
	uint32_t gid; /* group id */
	uint32_t mask; /* user permissions mask */
	uint32_t atime_lo; /* access time in Unix epoch time (low bytes) */
	uint32_t atime_hi; /* access time (high bytes) */
	uint32_t mtime_lo; /* modification time (low bytes) */
	uint32_t mtime_hi; /* modification time (high bytes) */
	uint32_t ctime_lo; /* creation time (low bytes) */
	uint32_t ctime_hi; /* creation time (high bytes) */
	uint32_t fsz; /* file size in bytes */
	uint32_t nblk; /* file size in blocks */
	uint8_t osimpl[8]; /* operating system implementation data */
	uint32_t blk[]; /* block index array */
} ECFS_PACKED ecfs_file_t;

/* convert a 32-bit value from host to little-endian byte order (or vice versa) */
static inline uint32_t ecfs_htold(uint32_t v) {

	uint32_t res;
	uint8_t *r = (uint8_t *)&res;
	r[0] = (uint8_t)(v & 0xff);
	r[1] = (uint8_t)((v >> 8) & 0xff);
	r[2] = (uint8_t)((v >> 16) & 0xff);
	r[3] = (uint8_t)((v >> 24) & 0xff);
	return res;
}

/* implementation values */
#ifdef ECFS_IMPL
int8_t ecfs_sig[] = ECFS_SIG;
#else
extern int8_t ecfs_sig[];
#endif

#endif /* ECFS_H */
