/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <stdatomic.h>
#include <math.h>

#define FUSE_USE_VERSION 30
#include <fuse.h>

#define ECFS_IMPL
#include "../include/ec/ecfs.h"
#include "../include/ec/mbr.h"

#define MIN(x, y) ((x) < (y)? (x): (y))

/* options */
enum {
	OPT_HELP = 0,
	OPT_FG,
	OPT_MBR,

	OPT_COUNT,

	OPT_HELP_BIT = 0x1,
	OPT_FG_BIT = 0x2,
	OPT_MBR_BIT = 0x4,
};
static uint32_t opt_flags = 0;

static const char *arg_device = NULL;
static const char *arg_mountp = NULL;

/* fs node */
#define NAMESZ 128

typedef enum ntype {
	NTYPE_STUB = 0,
	NTYPE_REG,
	NTYPE_DIR,
	NTYPE_SLINK,

	NTYPE_COUNT,
} ntype_t;

typedef enum nflag {
	NFLAG_FILLED = 0x1, /* directory already filled */
} nflag_t;

#define N_TRACKEXT 8 /* keep track of this many extension blocks */
typedef struct node {
	char name[NAMESZ]; /* name */
	uint32_t blk; /* file entry block */
	uint32_t ext[N_TRACKEXT]; /* extension blocks */
	struct node *parent; /* parent */
	struct node *first; /* first child */
	struct node *last; /* last child */
	struct node *prev; /* previous sibling */
	struct node *next; /* next sibling */
	struct node *ptr; /* pointer for symlinks and stubs */
	ntype_t type; /* type */
	nflag_t flags; /* flags */
	struct {
		bool open; /* is file open */
		int flags; /* fcntl flags */
		uint32_t *arr; /* block array */
		void *blk; /* block data */
		uint32_t bblk; /* block number */
		uint32_t i; /* index of block */
		uint32_t pos; /* position in array */
		uint32_t fsz; /* size of file */
		uint32_t ablk; /* array block (file entry or extension block) */
	} file; /* open file info */
} node_t;

static FILE *devfp; /* device file */
static void *blkbuf = NULL; /* block buffer */
static node_t *root; /* root node */
static ecfs_hblk_t hblk; /* head block */
static size_t fsbase = 0; /* base of file system */
static uint32_t blksz; /* block size */
static uint32_t nblk; /* number of blocks in volume */
static uint32_t *tlrm; /* top-level reservation map */
static uint8_t *brb; /* block reservation bitmap */
static uint32_t blk_tlrm; /* start tlrm block */
static uint32_t blk_brb; /* start brb block */
static uint32_t nblk_tlrm; /* number of tlrm blocks */
static uint32_t nblk_brb; /* number of brb blocks */
static uint32_t nblk_rsvd; /* number of initial reserved blocks */

/* read block */
static void read_blk(uint32_t blk, void *buf) {

	if (!devfp || blk >= nblk) return;

	fseek(devfp, ((long)fsbase << 9) + ((long)blk) * ((long)blksz), SEEK_SET);
	fread(buf, 1, blksz, devfp);
}

/* write block */
static void write_blk(uint32_t blk, void *buf) {

	if (!devfp || blk >= nblk) return;

	fseek(devfp, ((long)fsbase << 9) + ((long)blk) * ((long)blksz), SEEK_SET);
	fwrite(buf, 1, blksz, devfp);
}

/* allocate block */
static uint32_t alloc_blk(void) {

	uint32_t i = 0, j;
	for (; i < blksz * nblk_brb; i++) {

		bool found = false;
		for (j = 0; j < 8; j++) {

			if (!(brb[i] & (1 << j))) {

				found = true;
				break;
			}
		}
		if (found) break;
	}
	if (i >= blksz * nblk_brb) return 0;

	/* update brb */
	brb[i] |= (1 << j);

	uint32_t blk = i/blksz;
	write_blk(blk_brb+blk, (void *)brb + (size_t)blk * (size_t)blksz);

	/* update tlrm */
	tlrm[blk] = ecfs_htold(ecfs_htold(tlrm[blk]) + 1);

	blk = blk/(blksz >> 2);
	write_blk(blk_tlrm+blk, (void *)tlrm + (size_t)blk * (size_t)blksz);

	/* update head block */
	hblk.nblk_alloc = ecfs_htold(ecfs_htold(hblk.nblk_alloc) + 1);

	fseek(devfp, ((long)fsbase << 9) + ECFS_HBLK_START, SEEK_SET);
	fwrite(&hblk, 1, sizeof(hblk), devfp);

	return i * 8 + j;
}

/* free block */
static void free_blk(uint32_t blk) {

	if (blk >= nblk) return;

	uint32_t byte = blk >> 3; /* byte in brb */
	uint32_t bit = blk & 0x7; /* bit in byte */
	
	/* update brb */
	if (!(brb[byte] & (1 << bit)))
		return;
	brb[byte] &= ~(1 << bit);

	blk /= blksz;
	write_blk(blk_brb+blk, (void *)brb + (size_t)blk * (size_t)blksz);

	/* update tlrm */
	tlrm[blk] = ecfs_htold(ecfs_htold(tlrm[blk]) - 1);

	blk = blk/(blksz >> 2);
	write_blk(blk_tlrm+blk, (void *)tlrm + (size_t)blk * (size_t)blksz);

	/* update head block */
	hblk.nblk_alloc = ecfs_htold(ecfs_htold(hblk.nblk_alloc) - 1);

	fseek(devfp, ((long)fsbase << 9) + ECFS_HBLK_START, SEEK_SET);
	fwrite(&hblk, 1, sizeof(hblk), devfp);
}

static void node_getext(node_t *node);

/* create node */
static node_t *node_new(node_t *parent) {

	node_t *node = (node_t *)malloc(sizeof(node_t));
	memset(node, 0, sizeof(node_t));
	node->parent = parent;

	if (parent) {
		node->prev = parent->last;
		if (!parent->first) parent->first = node;
		if (parent->last) parent->last->next = node;
		parent->last = node;
	}
	return node;
}

/* fill directory with dirents */
static void node_filldir(node_t *node) {

	if (!node || node->type != NTYPE_DIR) return;

	/* read block */
	if (!blkbuf) blkbuf = malloc((size_t)blksz);
	read_blk(node->blk, blkbuf);

	ecfs_file_t *file = (ecfs_file_t *)blkbuf;

	uint32_t ndirents = ecfs_htold(file->nblk);
	uint32_t pos = sizeof(ecfs_file_t) >> 2; /* position in block array */
	uint32_t *array = (uint32_t *)blkbuf;

	for (uint32_t i = 0; i < ndirents; i++) {

		uint32_t blk = ecfs_htold(array[pos]);

		/* load next extension block */
		if (pos >= (blksz >> 2) - 1) {

			read_blk(blk, blkbuf);
			pos = 0;
			continue;
		}

		/* create node */
		if (blk) {

			node_t *child = node_new(node);
			child->blk = blk;
		}
		pos++;
	}

	/* read in basic info */
	node_t *cur = node->first;
	while (cur) {

		read_blk(cur->blk, blkbuf);
		memcpy(cur->name, file->name, 128);

		if (file->flags & ECFS_FLAG_REG) cur->type = NTYPE_REG;
		if (file->flags & ECFS_FLAG_DIR) cur->type = NTYPE_DIR;
		if (file->flags & ECFS_FLAG_SLINK) cur->type = NTYPE_SLINK;

		node_getext(cur);

		cur = cur->next;
	}

	node->flags |= NFLAG_FILLED;
}

/* get node extension blocks */
static void node_getext(node_t *node) {

	if (!node || !node->blk) return;

	/* read file entry */
	if (!blkbuf) blkbuf = malloc((size_t)blksz);
	read_blk(node->blk, blkbuf);

	uint32_t *arr = (uint32_t *)blkbuf;
	for (uint32_t i = 0; arr[(blksz >> 2) - 1] && i < N_TRACKEXT; i++) {

		node->ext[i] = ecfs_htold(arr[(blksz >> 2) - 1]);
		read_blk(node->ext[i], blkbuf);
	}
}

/* locate node from path */
#define PATHBUFSZ 512
static char pathbuf[PATHBUFSZ];

static node_t *node_resolve_full(const char *path, bool retlast) {

	node_t *node = root;
	size_t pathlen = strlen(path);
	size_t len = 0;
	
	while (path) {

		if (!node || node->type != NTYPE_DIR) return NULL;

		const char *end = strchr(path, '/');
		size_t size = end? (size_t)(end - path): pathlen - len;

		memset(pathbuf, 0, PATHBUFSZ);
		strncpy(pathbuf, path, MIN(size, PATHBUFSZ-1));

		path = end? end+1: NULL;
		len += size+1;
		if (!size) continue;

		if (!(node->flags & NFLAG_FILLED))
			node_filldir(node);

		node_t *cur = node->first;
		while (cur) {

			if (!strcmp(cur->name, pathbuf))
				break;
			cur = cur->next;
		}

		if (!cur) {

			if (retlast && !path) return node;
			else return NULL;
		}
		node = cur;
	}
	return node;
}

static inline node_t *node_resolve(const char *path) {

	return node_resolve_full(path, false);
}

/* get block at index */
#define N_ABLK(n, i) (!(i)? (n)->blk: (n)->ext[(i)-1])

static uint32_t node_getblk(node_t *node, uint32_t i) {

	i += sizeof(ecfs_file_t) >> 2;
	uint32_t m = (blksz >> 2) - 1;

	if (i >= (N_TRACKEXT+1) * m)
		return 0;
	uint32_t ablk = i / m;
	uint32_t pos = i % m;

	/* read array block */
	if (node->file.ablk != ablk) {

		read_blk(N_ABLK(node, ablk), node->file.arr);
		node->file.ablk = ablk;
	}

	return ecfs_htold(node->file.arr[pos]);
}

/* set block at index */
static int node_setblk(node_t *node, uint32_t i, uint32_t blk) {

	i += sizeof(ecfs_file_t) >> 2;
	const uint32_t m = (blksz >> 2) - 1;

	if (i >= (N_TRACKEXT+1) * m)
		return -ENOSPC;
	uint32_t ablk = i / m;
	uint32_t pos = i % m;

	/* allocate array blocks (skip the first as it is the file entry) */
	for (uint32_t j = 1; j <= ablk; j++) {
		if (!N_ABLK(node, j)) {

			uint32_t ablkn = alloc_blk();
			if (!ablkn) return -ENOSPC;

			read_blk(N_ABLK(node, j-1), node->file.arr);
			node->file.arr[m] = ecfs_htold(ablkn);
			write_blk(N_ABLK(node, j-1), node->file.arr);

			/* zero out extension block */
			memset(node->file.arr, 0, blksz);
			write_blk(N_ABLK(node, j), node->file.arr);

			node->ext[j-1] = ablkn;
		}
	}

	/* read array block */
	if (node->file.ablk != ablk) {

		read_blk(N_ABLK(node, ablk), node->file.arr);
		node->file.ablk = ablk;
	}

	/* update array block */
	if (node->file.arr[pos]) free_blk(ecfs_htold(node->file.arr[pos]));
	node->file.arr[pos] = ecfs_htold(blk);
	write_blk(N_ABLK(node, ablk), node->file.arr);

	return 0;
}

/* remove node */
static void node_free(node_t *node) {

	if (node->next) node->next->prev = node->prev;
	if (node->prev) node->prev->next = node->next;

	if (node->parent && node == node->parent->first)
		node->parent->first = node->next;
	if (node->parent && node == node->parent->last)
		node->parent->last = node->prev;

	if (node->file.blk) free(node->file.blk);
	if (node->file.arr) free(node->file.arr);

	node_t *cur = node->first;
	while (cur) {

		node_t *next = cur->next;
		node_free(cur);
		cur = next;
	}
	free(node);
}

/* load and check master boot record for first ecfs partition */
static int load_mbr(const char *prog) {

	ec_mbr_t mbr;
	fseek(devfp, EC_MBR_START, SEEK_SET);
	fread(&mbr, 1, sizeof(mbr), devfp);

	int p = 0;
	for (; p < 4; p++) {
		if (mbr.part[p].type == 0xec)
			break;
	}
	if (p >= 4) {

		fprintf(stderr, "%s: No valid EcFS partitions\n", prog);
		return -1;
	}

	fsbase = (size_t)ecfs_htold(mbr.part[p].start_lba);
	return 0;
}

/* load file system */
static int load_fs(const char *prog) {

	if ((opt_flags & OPT_MBR_BIT) && load_mbr(prog) < 0)
		return -1;

	fseek(devfp, ((long)fsbase << 9) + ECFS_HBLK_START, SEEK_SET);
	fread(&hblk, 1, sizeof(hblk), devfp);

	if (!!memcmp(hblk.signature, ecfs_sig, 4)) {

		fprintf(stderr, "%s: Not a valid EcFS file system\n", prog);
		return -1;
	}

	blksz = ecfs_htold(hblk.blksz);
	nblk = ecfs_htold(hblk.nblk);
	blk_tlrm = ecfs_htold(hblk.blk_tlrm);
	blk_brb = ecfs_htold(hblk.blk_brb);
	nblk_tlrm = ecfs_htold(hblk.nblk_tlrm);
	nblk_brb = ecfs_htold(hblk.nblk_brb);
	nblk_rsvd = ecfs_htold(hblk.nblk_rsvd);

	/* allocate and read tlrm and brb */
	tlrm = (uint32_t *)malloc((size_t)nblk_tlrm * (size_t)blksz);
	brb = (uint8_t *)malloc((size_t)nblk_brb * (size_t)blksz);

	for (uint32_t i = 0; i < nblk_tlrm; i++)
		read_blk(blk_tlrm+i, (void *)tlrm + (size_t)i * (size_t)blksz);
	for (uint32_t i = 0; i < nblk_brb; i++)
		read_blk(blk_brb+i, (void *)brb + (size_t)i * (size_t)blksz);

	/* create root node */
	root = node_new(NULL);

	root->blk = ecfs_htold(hblk.blk_root);
	root->type = NTYPE_DIR;

	node_getext(root);

	return 0;
}

/* acquire and release fs mutex lock */
static atomic_int fs_lock = 0;

static inline int fs_lock_acquire(void) {

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 10000;

	int i = 0;
	for (; i < 100000; i++) {

		if (fs_lock) nanosleep(&ts, NULL);
		else break;
	}
	if (i >= 100000) return -1;

	fs_lock = 1;
	return 0;
}

static inline void fs_lock_release(void) {

	fs_lock = 0;
}

#define FS_ACQUIRE() if (fs_lock_acquire() < 0) return -EAGAIN;
#define FS_RETURN(c) ({\
		fs_lock_release();\
		return c;\
	})

/* get attributes */
static int fs_getattr(const char *path, struct stat *st) {

	FS_ACQUIRE();

	node_t *node = node_resolve(path);
	if (!node) FS_RETURN(-ENOENT);

	/* read attributes */
	if (!blkbuf) blkbuf = malloc((size_t)blksz);
	read_blk(node->blk, blkbuf);

	ecfs_file_t *file = (ecfs_file_t *)blkbuf;

	st->st_mode = (mode_t)ecfs_htold(file->mask);
	st->st_nlink = 1;
	st->st_uid = (uid_t)ecfs_htold(file->uid);
	st->st_gid = (gid_t)ecfs_htold(file->gid);
	st->st_size = (off_t)ecfs_htold(file->fsz);
	st->st_atime = ((time_t)ecfs_htold(file->atime_lo) | ((time_t)ecfs_htold(file->atime_hi) << 32));
	st->st_mtime = ((time_t)ecfs_htold(file->mtime_lo) | ((time_t)ecfs_htold(file->mtime_hi) << 32));
	st->st_ctime = st->st_mtime;
	st->st_blksize = (blksize_t)blksz;
	st->st_blocks = (blkcnt_t)ecfs_htold(file->nblk);

	if (node->type == NTYPE_REG) st->st_mode |= S_IFREG;
	else if (node->type == NTYPE_DIR) st->st_mode |= S_IFDIR;
	else if (node->type == NTYPE_SLINK) st->st_mode |= S_IFLNK;

	FS_RETURN(0);
}

/* get extended attributes */
static int fs_getxattr(const char *path, const char *name, char *buf, size_t size) {

	return -ENODATA;
}

/* set extended attributes */
static int fs_setxattr(const char *path, const char *name, const char *buf, size_t size, int n) {

	return -ENODATA;
}

/* read directory entry */
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t fill, off_t off, struct fuse_file_info *file) {

	FS_ACQUIRE();

	node_t *node = node_resolve(path);
	if (!node) FS_RETURN(-ENOENT);
	if (node->type != NTYPE_DIR) FS_RETURN(-ENOTDIR);

	/* fill directory */
	if (!(node->flags & NFLAG_FILLED))
		node_filldir(node);

	off_t pos = 0;
	fill(buf, ".", NULL, 0);
	fill(buf, "..", NULL, 0);
	
	/* add children */
	node_t *cur = node->first;
	while (cur) {

		fill(buf, cur->name, NULL, 0);
		cur = cur->next;
	}

	FS_RETURN(0);
}

/* make file node */
static int fs_mkfile(const char *path, ntype_t ftype, uint32_t mask) {

	FS_ACQUIRE();

	node_t *node = node_resolve_full(path, true);
	if (!node) FS_RETURN(-ENOENT);
	if (node->type != NTYPE_DIR) FS_RETURN(-ENOTDIR);
	
	/* read file info */
	if (!blkbuf) blkbuf = malloc((size_t)blksz);
	read_blk(node->blk, blkbuf);

	ecfs_file_t *file = (ecfs_file_t *)blkbuf;

	uint32_t ndirents = ecfs_htold(file->nblk);
	uint32_t uid = ecfs_htold(file->uid);
	uint32_t gid = ecfs_htold(file->gid);

	uint32_t pos = sizeof(ecfs_file_t) >> 2;

	uint32_t *array = (uint32_t *)blkbuf;

	uint32_t extblk = node->blk;
	uint32_t i = 0;
	for (; i < ndirents; i++) {

		uint32_t blk = ecfs_htold(array[pos]);

		/* needs to load extension block */
		if (pos >= (blksz >> 2) - 1)
			FS_RETURN(-ENOSPC);

		if (!blk) break; /* free entry */
		pos++;
	}

	/* no free entry found */
	if (i >= ndirents) {

		/* allocate extension block */
		if (pos >= (blksz >> 2) - 1) {

			uint32_t oldblk = extblk;

			extblk = alloc_blk();
			if (!extblk) FS_RETURN(-ENOSPC);

			array[(blksz >> 2) - 1] = ecfs_htold(extblk);
			write_blk(oldblk, blkbuf);

			pos = 0;
			memset(blkbuf, 0, (size_t)blksz);
			ndirents++;
		}
		ndirents++;
	}

	/* allocate file entry block */
	uint32_t fblk = alloc_blk();
	if (!fblk) FS_RETURN(-ENOSPC);

	array[pos] = ecfs_htold(fblk);
	write_blk(extblk, blkbuf);

	memset(blkbuf, 0, (size_t)blksz);
	time_t tm = time(NULL);
	
	/* determine file name */
	const char *fname = strrchr(path, '/');
	if (!fname) fname = path; /* use full path */
	else fname++; /* remove '/' from name */
	strncpy(file->name, fname, 128);

	uint32_t atime_lo = ecfs_htold((uint32_t)(tm & 0xffffffff));
	uint32_t atime_hi = ecfs_htold((uint32_t)((tm >> 32) & 0xffffffff));

	/* fill file info */
	if (ftype == NTYPE_REG) file->flags |= ECFS_FLAG_REG;
	else if (ftype == NTYPE_DIR) file->flags |= ECFS_FLAG_DIR;
	else if (ftype == NTYPE_SLINK) file->flags |= ECFS_FLAG_SLINK;

	file->uid = ecfs_htold(uid);
	file->gid = ecfs_htold(gid);
	file->mask = ecfs_htold(mask);
	file->atime_lo = atime_lo;
	file->atime_hi = atime_hi;
	file->mtime_lo = atime_lo;
	file->mtime_hi = atime_hi;
	file->ctime_lo = atime_lo;
	file->ctime_hi = atime_hi;

	write_blk(fblk, blkbuf);

	/* update parent node */
	read_blk(node->blk, blkbuf);

	file->atime_lo = atime_lo;
	file->atime_hi = atime_hi;
	file->mtime_lo = atime_lo;
	file->mtime_hi = atime_hi;
	file->fsz = ecfs_htold(ndirents * blksz);
	file->nblk = ecfs_htold(ndirents);

	write_blk(node->blk, blkbuf);

	/* add child node */
	node_t *child = node_new(node);

	strncpy(child->name, fname, NAMESZ);
	child->blk = fblk;
	child->type = ftype;

	node_getext(child);

	FS_RETURN(0);
}

/* create regular file */
static int fs_mknod(const char *path, mode_t mode, dev_t dev) {

	if (!S_ISREG(mode)) return -ENOSYS;

	return fs_mkfile(path, NTYPE_REG, (uint32_t)mode & 0777);
}

/* create directory */
static int fs_mkdir(const char *path, mode_t mode) {

	return fs_mkfile(path, NTYPE_DIR, (uint32_t)mode & 0777);
}

/* open file */
static int fs_open(const char *path, struct fuse_file_info *finfo) {

	FS_ACQUIRE();

	node_t *node = node_resolve(path);
	if (!node) FS_RETURN(-ENOENT);

	if (node->type == NTYPE_DIR) FS_RETURN(-EISDIR);
	if (node->file.open) FS_RETURN(-EBUSY);
	if (finfo->flags & O_APPEND) FS_RETURN(-ENOSYS);
	
	/* open file */
	node->file.open = true;
	node->file.flags = finfo->flags;
	node->file.arr = (uint32_t *)malloc((size_t)blksz);
	node->file.blk = malloc((size_t)blksz);
	node->file.i = 0;
	node->file.bblk = 0;
	node->file.pos = sizeof(ecfs_file_t) >> 2;
	node->file.ablk = 0;

	read_blk(node->blk, node->file.arr);
	ecfs_file_t *file = (ecfs_file_t *)node->file.arr;

	node->file.fsz = ecfs_htold(file->fsz);
	
	FS_RETURN(0);
}

/* create and open file */
static int fs_create(const char *path, mode_t mode, struct fuse_file_info *finfo) {

	node_t *node = node_resolve(path);
	if (!node) {

		int res = fs_mknod(path, mode, 0);
		if (res < 0) return res;
	}
	return fs_open(path, finfo);
}

/* truncate file */
static int fs_truncate(const char *path, off_t offset) {

	FS_ACQUIRE();

	node_t *node = node_resolve(path);
	if (!node || !node->file.open) FS_RETURN(-ENOENT);

	if (offset > 0) FS_RETURN(-ENOSYS);

	/* truncate file */
	uint32_t fnblk = ECFS_ALIGN(node->file.fsz, blksz) / blksz;
	for (uint32_t i = 0; i < fnblk; i++)
		node_setblk(node, i, 0);
	node->file.fsz = 0;
	
	FS_RETURN(0);
}

/* close file */
static int fs_release(const char *path, struct fuse_file_info *finfo) {

	FS_ACQUIRE();

	node_t *node = node_resolve(path);
	if (!node) FS_RETURN(-ENOENT);

	if (!node->file.open) FS_RETURN(-EBADF);

	/* update file entry */
	read_blk(node->blk, node->file.arr);
	ecfs_file_t *file = (ecfs_file_t *)node->file.arr;

	if (node->file.flags & O_ACCMODE != O_RDONLY) {

		file->fsz = ecfs_htold(node->file.fsz);
		file->nblk = ecfs_htold(ECFS_ALIGN(node->file.fsz, blksz) / blksz);
	}

	time_t tm = time(NULL);
	uint32_t tm_lo = ecfs_htold((uint32_t)(tm & 0xffffffff));
	uint32_t tm_hi = ecfs_htold((uint32_t)((tm >> 32) & 0xffffffff));

	file->atime_lo = tm_lo;
	file->atime_hi = tm_hi;
	file->mtime_lo = tm_lo;
	file->mtime_hi = tm_hi;

	write_blk(node->blk, node->file.arr);

	/* clean up resources */
	node->file.open = false;
	free(node->file.arr);
	free(node->file.blk);
	node->file.arr = NULL;
	node->file.blk = NULL;

	FS_RETURN(0);
}

/* read from file */
static int fs_read(const char *path, char *buf, size_t nbytes, off_t offset, struct fuse_file_info *info) {

	FS_ACQUIRE();

	node_t *node = node_resolve(path);
	if (!node) FS_RETURN(-ENOENT);

	if (!node->file.open || node->file.flags & O_ACCMODE == O_WRONLY) FS_RETURN(-EPERM);

	if (offset < 0 || (uint32_t)offset >= node->file.fsz) FS_RETURN(EOF);

	/* copy bytes */
	size_t count = 0;
	for (; count < nbytes; count++) {

		size_t pos = (size_t)offset + count;
		if (pos >= (size_t)node->file.fsz) FS_RETURN(count);

		/* read next block */
		uint32_t bidx = pos / blksz;
		if (!node->file.bblk || bidx != node->file.i) {

			node->file.i = bidx;
			node->file.bblk = node_getblk(node, bidx);
			if (!node->file.bblk) FS_RETURN(count);

			read_blk(node->file.bblk, node->file.blk);
		}

		buf[count] = ((char *)node->file.blk)[pos % blksz];
	}
	FS_RETURN(count);
}

/* write to file */
static int fs_write(const char *path, const char *buf, size_t nbytes, off_t offset, struct fuse_file_info *info) {

	FS_ACQUIRE();

	node_t *node = node_resolve(path);
	if (!node) FS_RETURN(-ENOENT);

	if (!node->file.open || node->file.flags & O_ACCMODE == O_RDONLY) FS_RETURN(-EPERM);

	/* copy bytes */
	size_t count = 0, pos = 0;
	for (; count < nbytes; count++) {

		pos = (size_t)offset + count;

		/* write previous block and read next block */
		uint32_t bidx = pos / blksz;
		if (!node->file.bblk || bidx != node->file.i) {

			if (node->file.bblk)
				write_blk(node->file.bblk, node->file.blk);

			node->file.i = bidx;
			node->file.bblk = node_getblk(node, bidx);
			if (!node->file.bblk) {

				uint32_t blk = alloc_blk();
				if (!blk) FS_RETURN(-ENOSPC);

				int code = node_setblk(node, bidx, blk);
				if (code < 0) FS_RETURN(code);

				node->file.bblk = blk;
			}

			read_blk(node->file.bblk, node->file.blk);
		}

		((char *)node->file.blk)[pos % blksz] = buf[count];
	}
	if (count && node->file.bblk) write_blk(node->file.bblk, node->file.blk);

	if (pos >= (size_t)node->file.fsz)
		node->file.fsz = (size_t)pos+1;

	FS_RETURN(count);
}

/* clean up resources */
static void cleanup(void) {

	if (blkbuf) {
		free(blkbuf);
		blkbuf = NULL;
	}
	if (brb) {
		free(brb);
		brb = NULL;
	}
	if (tlrm) {
		free(tlrm);
		tlrm = NULL;
	}
	if (root) {
		node_free(root);
		root = NULL;
	}
	if (devfp) {
		fclose(devfp);
		devfp = NULL;
	}
}

static void fs_destroy(void *p) {

	cleanup();
}

/* parse arguments */
static int parse_args(int argc, const char **argv) {

	for (int i = 1; i < argc; i++) {

		const char *arg = argv[i];
		if (arg[0] == '-') {

			/* print help message */
			if (!strcmp(arg, "-h") || !strcmp(arg, "--help"))
				opt_flags |= OPT_HELP_BIT;

			/* run in foreground */
			else if (!strcmp(arg, "-f") || !strcmp(arg, "--foreground"))
				opt_flags |= OPT_FG_BIT;

			/* check mbr */
			else if (!strcmp(arg, "-m") || !strcmp(arg, "--mbr"))
				opt_flags |= OPT_MBR_BIT;

			/* unknown */
			else {

				fprintf(stderr, "%s: Unrecognized option '%s'\n", argv[0], arg);
				return -1;
			}
		}

		/* generic argument */
		else if (!arg_device) arg_device = arg;

		else if (!arg_mountp) arg_mountp = arg;

		else {

			fprintf(stderr, "%s: Unexpected argument '%s'\n", argv[0], arg);
			return -1;
		}
	}

	/* final checks */
	if (opt_flags & OPT_HELP_BIT) return 0;

	if (!arg_device) {

		fprintf(stderr, "%s: Expected device file\n", argv[0]);
		return -1;
	}
	else if (!arg_mountp) {

		fprintf(stderr, "%s: Expected mount point\n", argv[0]);
		return -1;
	}
	return 0;
}

/* print help/usage info */
static void print_help(const char *prog) {

	fprintf(stderr, "Usage: %s <device> <mount point> [options]\n\nOptions:\n"
			"\t-h|--help        Show this help message\n"
			"\t-f|--foreground  Run in the foreground\n"
			"\t-m|--mbr         Check master boot record for EcFS partition\n",
			prog);
}

/* functions */
static struct fuse_operations fs_ops = {
	.getattr = fs_getattr,
	.getxattr = fs_getxattr,
	.setxattr = fs_setxattr,
	.readdir = fs_readdir,
	.mknod = fs_mknod,
	.mkdir = fs_mkdir,
	.open = fs_open,
	.create = fs_create,
	.truncate = fs_truncate,
	.read = fs_read,
	.write = fs_write,
	.release = fs_release,
	.destroy = fs_destroy,
};

/* run */
static int run(int argc, const char **argv) {

	if (parse_args(argc, argv) < 0)
		return 1;
	if (opt_flags & OPT_HELP_BIT) {

		print_help(argv[0]);
		return 0;
	}

	/* open device */
	devfp = fopen(arg_device, "rb+");
	if (!devfp) {

		fprintf(stderr, "%s: Can't open '%s': %s\n", argv[0], arg_device, strerror(errno));
		return 1;
	}

	/* load file system */
	if (load_fs(argv[0]) < 0) return 1;

	/* construct fake arguments */
	uint32_t fg = (opt_flags >> OPT_FG) & 0x1;

	const char *fake_argv[] = {argv[0], arg_mountp, "-o", "sync", fg? "-f": NULL, NULL};
	int fake_argc = (sizeof(fake_argv)/sizeof(fake_argv[0]))-2+fg;

	return fuse_main(fake_argc, (char **)fake_argv, &fs_ops, NULL);
}

int main(int argc, const char **argv) {

	int code = run(argc, argv);
	if (code) cleanup();
	return code;
}
