#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/device.h>
#include <kernel/fs/ecfs.h>

#define ECFS_IMPL
#include <ec/ecfs.h>

mbr_driver_t mbrfs = {
	.fstype = MBR_FS_ECFS,
	.mount = ecfs_mbr_mount,
};
DRIVERINFO(ecfs, DRIVERINFO_MBRFS, &mbrfs);

/* file system info */
struct ecfs_fs_info {
	device_t *dev; /* device */
	mbr_ent_t *part; /* partition */
	union {
		ecfs_hblk_t hb; /* head block */
		uint8_t hbpad[ECFS_HBLK_SIZE]; /* padding for head block */
	};
	uint32_t *tlrm; /* top-level reservation map */
	uint8_t *brb; /* block reservation bitmap */
	void *block; /* block buffer */
	bool held; /* file system busy */
};

/* open file info */
struct ecfs_file_info {
	uint32_t bblk; /* block id */
	uint32_t bidx; /* block index */
	uint32_t ablki; /* array block index */
	void *bdata; /* block data */
	uint32_t *adata; /* array data */
	ecfs_file_t file; /* file data */
};

/* extra node info */
#define TRACKEXT 8

struct ecfs_node {
	fs_node_t base;
	uint32_t ext[TRACKEXT]; /* extension blocks */
};

static inline fs_node_t *node_new(fs_node_t *parent, uint32_t flags) {

	return fs_node_new_ext(parent, flags, sizeof(struct ecfs_node));
}

/* read block from fs volume */
static void read_block(struct ecfs_fs_info *info, uint32_t blk, uint8_t *buf) {

	uint32_t cnt = info->hb.blksz >> 9;
	device_storage_read(info->dev, info->part->start_lba + blk * cnt, cnt, buf);
}

/* write block to fs volume */
static void write_block(struct ecfs_fs_info *info, uint32_t blk, uint8_t *buf) {

	uint32_t cnt = info->hb.blksz >> 9;
	device_storage_write(info->dev, info->part->start_lba + blk * cnt, cnt, buf);
}

/* translate type info */
static uint32_t translate_type_flags(uint32_t flags) {

	uint32_t out = 0;
	if (flags & ECFS_FLAG_REG) out |= FS_FILE;
	if (flags & ECFS_FLAG_DIR) out |= FS_DIRECTORY;
	if (flags & ECFS_FLAG_SLINK) out |= FS_SYMLINK;

	return out;
}

/* read node info */
static void read_node_info(fs_node_t *node, fs_dirent_t *dent, uint8_t *bdata) {

	struct ecfs_node *enode = (struct ecfs_node *)node;
	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;

	read_block(info, node->inode, bdata);
	ecfs_file_t *file = (ecfs_file_t *)bdata;

	/* set info */
	node->mask = file->mask;
	node->uid = file->uid;
	node->gid = file->gid;
	node->flags = translate_type_flags(file->flags);
	node->len = file->fsz;
	node->impl = file->nblk; /* impl used here as number of blocks */

	if (dent) strncpy(dent->name, file->name, FS_NAMESZ);

	/* read extension blocks */
	uint32_t *arr = (uint32_t *)bdata;
	for (uint32_t i = 0; i < TRACKEXT && arr[info->hb.blksz >> 2]; i++) {

		uint32_t blk = arr[info->hb.blksz >> 2];
		enode->ext[i] = blk;

		read_block(info, blk, bdata);
	}
}

/* get block at index */
#define ABLK(n, i) (!(i)? (n)->base.inode: (n)->ext[(i)-1])

static uint32_t get_node_block(fs_node_t *node, uint32_t i, uint32_t *ablkp, uint8_t *adata) {

	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;
	struct ecfs_node *enode = (struct ecfs_node *)node;

	i += sizeof(ecfs_file_t) >> 2;
	uint32_t m = (info->hb.blksz >> 2) - 1;

	if (i >= (TRACKEXT+1) * m)
		return 0;
	uint32_t ablk = i / m;
	uint32_t pos = i % m;

	/* read array block */
	if (*ablkp != ablk) {

		read_block(info, ABLK(enode, ablk), adata);
		*ablkp = ablk;
	}
	return ((uint32_t *)adata)[pos];
}

/* read from file */
static kssize_t ecfs_read(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;
	struct ecfs_file_info *file = (struct ecfs_file_info *)node->odata;

	info->held = true;

	/* copy bytes */
	size_t count = 0;
	for (; count < nbytes; count++) {

		size_t pos = offset + count;
		if (pos >= node->len) {

			info->held = false;
			return count;
		}

		/* read next block */
		uint32_t bidx = ((uint32_t)pos / info->hb.blksz);
		if (!file->bblk || bidx != file->bidx) {

			file->bidx = bidx;

			file->bblk = get_node_block(node, bidx, &file->ablki, (uint8_t *)file->adata);
			if (!file->bblk) {

				info->held = false;
				return count;
			}
			read_block(info, file->bblk, file->bdata);
		}

		buf[count] = ((uint8_t *)file->bdata)[pos % info->hb.blksz];
	}
	info->held = false;
	return count;
}

/* write to file */
static kssize_t ecfs_write(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	return 0;
}

/* open file */
static void ecfs_open(fs_node_t *node, uint32_t flags) {

	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;
	info->held = true;

	node->odata = kmalloc(sizeof(struct ecfs_file_info));
	struct ecfs_file_info *file = (struct ecfs_file_info *)node->odata;

	file->bblk = 0;
	file->bidx = 0;
	file->ablki = 0;
	file->bdata = kmalloc(info->hb.blksz);
	file->adata = kmalloc(info->hb.blksz);

	read_block(info, node->inode, (uint8_t *)file->adata);
	memcpy(&file->file, file->adata, sizeof(ecfs_file_t));

	info->held = false;
}

/* close file */
static void ecfs_close(fs_node_t *node) {
	
	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;
	info->held = true;

	struct ecfs_file_info *file = (struct ecfs_file_info *)node->odata;
	kfree(file->bdata);
	kfree(file->adata);

	kfree(file);
	node->odata = NULL;
	info->held = false;
}

/* filldir */
static bool ecfs_filldir(fs_node_t *node) {

	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;
	info->held = true;

	/* read file info */
	uint8_t *bdata = (uint8_t *)info->block;

	read_block(info, node->inode, bdata);
	ecfs_file_t *file = (ecfs_file_t *)info->block;

	uint32_t nblk = file->nblk;
	uint32_t ablki = 0;

	fs_dirent_t *dent = fs_dirent_new(".");
	fs_node_add_dirent(node, dent);
	dent = fs_dirent_new("..");
	fs_node_add_dirent(node, dent);

	/* read file entries */
	uint8_t *dentbuf = (uint8_t *)kmalloc(info->hb.blksz);

	for (uint32_t i = 0; i < nblk; i++) {

		uint32_t blk = get_node_block(node, i, &ablki, bdata);
		if (!blk) continue;

		/* create node */
		dent = fs_dirent_new(NULL);

		fs_node_t *child = node_new(node, 0);
		child->inode = blk;
		dent->node = child;

		fs_node_add_dirent(node, dent);

		/* read file info */
		read_node_info(child, dent, dentbuf);
	}
	kfree(dentbuf);

	info->held = false;
	return true;
}

/* check if resource is busy */
static bool ecfs_isheld(fs_node_t *node) {

	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;
	return node->held || info->held || info->dev->held;
}

/* get file info */
static void ecfs_stat(fs_node_t *node, ec_stat_t *st) {

	struct ecfs_fs_info *info = (struct ecfs_fs_info *)node->data;
	info->held = true;

	read_block(info, node->inode, (uint8_t *)info->block);
	ecfs_file_t *file = (ecfs_file_t *)info->block;

	st->atime = ((long long)file->atime_hi << 32) | (long long)file->atime_lo;
	st->mtime = ((long long)file->mtime_hi << 32) | (long long)file->mtime_lo;
	st->ctime = ((long long)file->ctime_hi << 32) | (long long)file->ctime_lo;

	info->held = false;
}

/* mount file system */
extern fs_node_t *ecfs_mbr_mount(fs_node_t *mountp, device_t *dev, mbr_ent_t *part) {

	if (mountp->ptr) return NULL;

	struct ecfs_fs_info *info = kmalloc(sizeof(struct ecfs_fs_info));

	/* read head block */
	device_storage_read(dev, part->start_lba + 2, 2, info->hbpad);
	if (!!memcmp(info->hb.signature, ecfs_sig, 4)) {

		kfree(info);
		kprintf(LOG_WARNING, "[ecfs] Invalid EcFS signature");
		return NULL;
	}

	info->dev = dev;
	info->part = part;
	info->block = kmalloc(info->hb.blksz);

	/* load reservation maps */
	info->tlrm = (uint32_t *)kmalloc(info->hb.blksz * info->hb.nblk_tlrm);
	info->brb = (uint8_t *)kmalloc(info->hb.blksz * info->hb.nblk_brb);

	for (uint32_t i = 0; i < info->hb.nblk_tlrm; i++)
		read_block(info, info->hb.blk_tlrm + i, (uint8_t *)info->tlrm + i * info->hb.blksz);
	for (uint32_t i = 0; i < info->hb.nblk_brb; i++)
		read_block(info, info->hb.blk_brb + i, info->brb + i * info->hb.blksz);

	/* create and fill root node */
	fs_node_t *node = node_new(NULL, FS_DIRECTORY);
	node->data = info;
	node->inode = info->hb.blk_root;
	node->parent = mountp->parent;

	read_node_info(node, NULL, (uint8_t *)info->block);

	/* operations */
	node->read = ecfs_read;
	node->write = ecfs_write;
	node->open = ecfs_open;
	node->close = ecfs_close;
	node->filldir = ecfs_filldir;
	node->isheld = ecfs_isheld;
	node->stat = ecfs_stat;

	mountp->ptr = node;
	return node;
}
