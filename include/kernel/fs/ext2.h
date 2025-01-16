#ifndef ECLAIR_FS_EXT2_H
#define ECLAIR_FS_EXT2_H

#include <kernel/types.h>
#include <kernel/driver/device.h>
#include <kernel/vfs/fs.h>
#include <kernel/fs/mbr.h>

/* superblock */
typedef struct ext2_superblock {
	uint32_t ninodes; /* total number of inodes */
	uint32_t nblocks; /* total number of blocks */
	uint32_t nrootblocks; /* number of superuser reserved blocks */
	uint32_t nfreeblocks; /* total number of unallocated blocks */
	uint32_t nfreeinodes; /* total number of unallocated inodes */
	uint32_t supblock; /* block of number of superblock */
	uint32_t blocksize; /* block size */
	uint32_t fragsize; /* fragment size */
	uint32_t nbgblocks; /* number of blocks in block group */
	uint32_t nbgfrags; /* number of fragments in block group */
	uint32_t nbginodes; /* number of inodes in block group */
	uint32_t lmtime; /* last mount time */
	uint32_t lwtime; /* last written time */
	uint16_t nfsckmount0; /* number of mounts since last consistency check */
	uint16_t nfsckmount1; /* number of mounts allowed before consistency check */
	uint16_t ext2sig; /* file system signature */
	uint16_t state; /* file system state */
	uint16_t errmethod; /* error handling method */
	uint16_t vminor; /* minor portion of version number */
	uint32_t lfscktime; /* time of last consistency check */
	uint32_t fsckintv; /* interval between forced consistency checks */
	uint32_t sysid; /* id of operating system that created the volume */
	uint32_t vmajor; /* major portion of version number */
	uint16_t rsvduid; /* user id that can use reserved blocks */
	uint16_t rsvdgid; /* group id that can use reserved blocks */

	/* extended fields (ver >= 1) */
	uint32_t ext_ffreeinode; /* first free inode */
	uint16_t ext_inodesize; /* size of inode */
	uint16_t ext_bgsupblock; /* block group that superblock is part of */
	uint32_t ext_optfeats; /* optional features */
	uint32_t ext_reqfeats; /* required features */
	uint32_t ext_writefeats; /* write features */
	char ext_fsid[16]; /* filesystem id */
	char ext_volname[16]; /* volume name */
	char ext_lmountpath[64]; /* last mount path */
	uint32_t ext_compalg; /* compression algorithms */
	uint8_t ext_nbprefile; /* number of blocks to preallocate for files */
	uint8_t ext_nbpredir; /* number of blocks to preallocate for directories */
	uint16_t ext_unused; /* unused */
	char ext_journalid[16]; /* journal id */
	uint32_t ext_jinode; /* journal inode */
	uint32_t ext_jdev; /* journal device */
	uint32_t ext_headorph; /* head of orphan inode list */
} __attribute__((packed)) ext2_superblock_t;

/* optional features */
#define EXT2_OPTFEAT_PREALLOC_DIR_BLOCKS 0x1
#define EXT2_OPTFEAT_AFS_SERV_INODES 0x2
#define EXT2_OPTFEAT_HAS_JOURNAL 0x4
#define EXT2_OPTFEAT_INODE_EXT_ATTRS 0x8
#define EXT2_OPTFEAT_CAN_RESIZE 0x10
#define EXT2_OPTFEAT_DIR_HASH_INDEX 0x20

#define EXT2_OPTFEAT_ALL (EXT2_OPTFEAT_REALLOC_DIR_BLOCKS | EXT2_OPTFEAT_AFS_SERV_INODES | EXT2_OPTFEAT_HAS_JOURNAL | EXT2_OPTFEAT_INODE_EXT_ATTRS | EXT2_OPTFEAT_CAN_RESIZE | EXT2_OPTFEAT_DIR_HASH_INDEX)

/* required flags */
#define EXT2_REQFEAT_USE_COMPRESS 0x1
#define EXT2_REQFEAT_DIRENT_TYPE 0x2
#define EXT2_REQFEAT_REPLAY_JOURNAL 0x4
#define EXT2_REQFEAT_USE_JOURNAL_DEV 0x8

#define EXT2_REQFEAT_ALL (EXT2_REQFEAT_USE_COMPRESS | EXT2_REQFEAT_DIRENT_TYPE | EXT2_REQFEAT_REPLAY_JOURNAL | EXT2_REQFEAT_USE_JOURNAL_DEV)

/* block group descriptor */
typedef struct ext2_bg_descriptor {
	uint32_t busgbitmap; /* block usage bitmap */
	uint32_t iusgbitmap; /* inode usage bitmap */
	uint32_t binodetab; /* inode table */
	uint16_t nfreeblocks; /* number of free blocks */
	uint16_t nfreeinodes; /* number of free inodes */
	uint16_t ndirs; /* number of directories */
	char pad[14]; /* padding */
} __attribute__((packed)) ext2_bg_descriptor_t;

/* inode */
typedef struct ext2_inode {
	uint16_t type; /* type and permissions */
	uint16_t uid; /* user id */
	uint32_t losize; /* lower 32 bits of size */
	uint32_t latime; /* last access time */
	uint32_t lctime; /* last creation time */
	uint32_t lmtime; /* last modification time */
	uint32_t deltime; /* deletion time */
	uint16_t gid; /* group id */
	uint16_t nlinks; /* number of hard links to inode */
	uint32_t nsectors; /* number of sectors used by file */
	uint32_t flags; /* file flags */
	uint32_t osimpl0; /* os specific value */
	uint32_t dbptr[12]; /* first 12 direct pointers */
	uint32_t sibptr; /* singly indirect block pointer */
	uint32_t dibptr; /* doubly indirect block pointer */
	uint32_t tibptr; /* triply indirect block pointer */
	uint32_t gennum; /* generation number */
	uint32_t ext_attrblk; /* extended attribute block */
	uint32_t ext_hisize; /* high 32 bits of size */
	uint32_t bfrag; /* block address of fragment */
	uint8_t osimpl1[12]; /* os specific value */
} __attribute__((packed)) ext2_inode_t;

/* file types */
#define EXT2_FIFO 0x1000
#define EXT2_CHARDEV 0x2000
#define EXT2_DIR 0x4000
#define EXT2_BLKDEV 0x6000
#define EXT2_REG 0x8000
#define EXT2_SLINK 0xA000
#define EXT2_SOCK 0xC000

/* directory entry */
typedef struct ext2_dirent {
	uint32_t inode; /* inode of file */
	uint16_t entsize; /* total entry size */
	uint8_t lonamelen; /* low byte of name length */
	uint8_t type; /* type indicator or high byte of name length */
	char name[]; /* name characters */
} __attribute__((packed)) ext2_dirent_t;

/* functions */
extern fs_node_t *ext2_mbr_mount(fs_node_t *mountp, device_t *dev, mbr_ent_t *part); /* mount ext2 filesystem */

#endif /* ECLAIR_FS_EXT2_H */
