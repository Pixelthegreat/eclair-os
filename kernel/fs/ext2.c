#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/mm/heap.h>
#include <kernel/vfs/fs.h>
#include <kernel/fs/ext2.h>

#define EXT2_BLOCK_GROUP(info, inode) (((inode)-1) / (info)->sb.nbginodes)
#define EXT2_INDEX(info, inode) (((inode)-1) % (info)->sb.nbginodes)
#define EXT2_CONT_BLOCK(info, idx) (((idx) * (info)->inodesize) / (info)->blocksize)

/* file system info */
struct ext2_fs_info {
	device_t *dev; /* device */
	mbr_ent_t *part; /* partition info */
	union {
		ext2_superblock_t sb; /* superblock */
		uint8_t sbpad[1024]; /* padding for superblock */
	};
	size_t blocksize; /* size of blocks */
	size_t inodesize; /* size of inodes */
	uint32_t nbgs; /* number of block group */
	ext2_bg_descriptor_t *bgdt; /* block group descriptor table */
	uint32_t nbub; /* number of blocks per block usage bitmap */
	void *bub; /* block usage bitmap */
	uint32_t curblock; /* current read block */
	void *block; /* block data */
	bool held; /* file system busy */
};

/* open file info */
struct ext2_file_info {
	uint32_t bblk; /* block id */
	uint32_t bidx; /* block index */
	void *bdata; /* currently read block */
	void *bpdata; /* block pointer data */
	ext2_inode_t inode; /* inode data */
};

/* translate ext2 inode type */
static uint32_t ext2_translate_type(uint32_t type) {

	uint32_t out = 0;
	if (type & EXT2_FIFO) out |= FS_PIPE;
	if (type & EXT2_DIR) out |= FS_DIRECTORY;
	if (type & EXT2_REG) out |= FS_FILE;
	if (type & EXT2_SLINK) out |= FS_SYMLINK;
	if (type & EXT2_SOCK) out |= FS_SOCKET;
	return out;
}

/* verify ext2 filesystem */
static bool ext2_verify_sb(ext2_superblock_t *sb) {

	if (sb->ext2sig != 0xef53) return false;
	if (sb->vmajor >= 1 && (sb->ext_reqfeats & (EXT2_REQFEAT_USE_COMPRESS | EXT2_REQFEAT_REPLAY_JOURNAL | EXT2_REQFEAT_USE_JOURNAL_DEV))) return false;
	return true;
}

/* read block */
static void ext2_read_block(struct ext2_fs_info *info, uint32_t block, void *buf) {

	uint32_t lba = info->part->start_lba + block * (info->blocksize >> 9);
	device_storage_read(info->dev, lba, info->blocksize >> 9, buf);
}

/* read cached block */
static void ext2_read_cached_block(struct ext2_fs_info *info, uint32_t block) {

	if (info->curblock == block) return;

	/* allocate buffer */
	if (!info->block) info->block = kmalloc(info->blocksize);
ext2_read_block(info, block, info->block);
}

/* write block */
static void ext2_write_block(struct ext2_fs_info *info, uint32_t block, void *buf) {

	uint32_t lba = info->part->start_lba + block * (info->blocksize >> 9);
	device_storage_write(info->dev, lba, info->blocksize >> 9, buf);
}

/* load block group descriptor */
static void ext2_load_bgdt(struct ext2_fs_info *info) {

	uint32_t block = (info->blocksize == 1024)? 2: 1;
	uint32_t nbgs = info->sb.nblocks / info->sb.nbgblocks + 1;
	uint32_t nblocks = (nbgs * sizeof(ext2_bg_descriptor_t)) / info->blocksize + 1;
	
	/* read blocks */
	void *buf = kmalloc(info->blocksize * nblocks);
	for (uint32_t i = 0; i < nblocks; i++)
		ext2_read_block(info, block + i, buf + i * info->blocksize);

	info->nbgs = nbgs;
	info->bgdt = (ext2_bg_descriptor_t *)buf;

	/* read block usage bitmaps */
	uint32_t nbub = info->sb.nbgblocks / (info->blocksize * 8) + 1;

	buf = kmalloc(info->blocksize * nblocks * nbub);
	for (uint32_t i = 0; i < nblocks; i++) {

		for (uint32_t j = 0; j < nbub; j++)
			ext2_read_block(info, info->bgdt[i].busgbitmap+j, buf + (i * nbub + j) * info->blocksize);
	}

	info->nbub = nbub;
	info->bub = buf;
}

/* flush block group descriptor */
static void ext2_flush_bgdt_entry(struct ext2_fs_info *info, uint32_t bg) {

	uint32_t idx = bg / info->sb.nbgblocks;
	uint32_t block = ((info->blocksize == 1024)? 2: 1) + idx;

	ext2_write_block(info, block, (void *)info->bgdt + idx * info->blocksize);
}

/* flush block usage bitmap for block group */
static void ext2_flush_bgdt_bub(struct ext2_fs_info *info, uint32_t bg, uint32_t bub) {

	uint32_t idx = bg / info->sb.nbgblocks;
	uint32_t block = info->bgdt[idx].busgbitmap + (idx * info->nbub + bub);

	ext2_write_block(info, block, info->bub + (idx * info->nbub + bub) * info->blocksize);
}

/* allocate block */
static uint32_t ext2_allocate_block(struct ext2_fs_info *info) {

	uint32_t bg = 0;
	for (; bg < info->nbgs; bg++) {
		if (info->bgdt[bg].nfreeblocks > 0)
			break;
	}
	if (bg >= info->nbgs) return 0;

	/* search bitmap */
	uint8_t *bub = (uint8_t *)info->bub + (bg * info->nbub);
	uint32_t j = 0, k = 0;
	uint8_t b = 0;

	for (; j < info->blocksize * info->nbub; j++) {

		b = bub[j];
		bool found = false;
		for (k = 0; k < 8; k++) {
			if (!(b & (1 << k))) {
				found = true;
				break;
			}
		}
		if (found) break;
	}
	if (j >= info->blocksize * info->nbub) return 0;

	/* update bitmap */
	b |= (1 << k);
	bub[j] = b;
	info->bgdt[bg].nfreeblocks--;

	ext2_flush_bgdt_bub(info, bg, j / info->blocksize);
	ext2_flush_bgdt_entry(info, bg);

	return (bg * info->sb.nbgblocks) + (j * 8) + k;
}

/* free block */
static void ext2_free_block(struct ext2_fs_info *info, uint32_t block) {

	uint32_t bg = block / info->sb.nbgblocks;
	uint32_t idx = block % info->sb.nbgblocks;
	uint32_t byte = idx / 8;
	uint32_t bit = idx % 8;

	uint8_t *bub = (uint8_t *)info->bub + (bg * info->nbub);
	uint8_t b = bub[byte];

	if (!(b & (1 << bit)))
		return;

	/* update bitmap */
	b &= ~(1 << bit);
	bub[byte] = b;
	info->bgdt[bg].nfreeblocks++;

	ext2_flush_bgdt_bub(info, bg, byte / info->blocksize);
	ext2_flush_bgdt_entry(info, bg);
}

/* read inode */
static void ext2_read_inode(struct ext2_fs_info *info, uint32_t inode, ext2_inode_t *inodebuf) {

	uint32_t bg = EXT2_BLOCK_GROUP(info, inode);
	uint32_t idx = EXT2_INDEX(info, inode);
	uint32_t cont = info->bgdt[bg].binodetab + EXT2_CONT_BLOCK(info, idx);

	/* read block */
	ext2_read_cached_block(info, cont);

	/* copy contents */
	uint32_t bidx = idx % (info->blocksize / info->inodesize);
	memcpy(inodebuf, info->block + bidx * info->inodesize, sizeof(ext2_inode_t));
}

/* write inode */
static void ext2_write_inode(struct ext2_fs_info *info, uint32_t inode, ext2_inode_t *inodebuf) {

	uint32_t bg = EXT2_BLOCK_GROUP(info, inode);
	uint32_t idx = EXT2_INDEX(info, inode);
	uint32_t cont = info->bgdt[bg].binodetab + EXT2_CONT_BLOCK(info, idx);

	/* read block */
	ext2_read_cached_block(info, cont);

	/* copy contents */
	uint32_t bidx = idx % (info->blocksize / info->inodesize);
	memcpy(info->block + bidx * info->inodesize, inodebuf, sizeof(ext2_inode_t));

	/* write block */
	ext2_write_block(info, cont, info->block);
}

/* read block from inode data */
/* todo: support doubly and triply indirect block pointers */
static uint32_t ext2_read_inode_block(struct ext2_fs_info *info, ext2_inode_t *inode, uint32_t idx, void *buf) {

	if (idx < 12) {

		uint32_t block = inode->dbptr[idx];
		if (!block) return 0;

		ext2_read_block(info, block, buf);
		return block;
	}

	/* singly indirect block pointer */
	else if (idx < 12 + (info->blocksize >> 2)) {

		if (!inode->sibptr) return 0;
		ext2_read_block(info, inode->sibptr, buf);

		uint32_t block = ((uint32_t *)buf)[idx-12];
		if (!block) return 0;

		ext2_read_block(info, block, buf);
		return block;
	}

	return 0;
}

/* set block from inode data and read */
static uint32_t ext2_set_inode_block(struct ext2_fs_info *info, uint32_t ninode, ext2_inode_t *inode, uint32_t idx, void *buf) {

	uint32_t block = 0;
	bool update = false; /* update to inode */
	if (idx < 12) {

		block = inode->dbptr[idx];
		if (!block) {

			block = ext2_allocate_block(info);
			if (!block) return 0;

			inode->dbptr[idx] = block;
			inode->nsectors += (info->blocksize >> 9);
			update = true;
		}

		ext2_read_block(info, block, buf);
	}

	/* singly indirect block pointer */
	else if (idx < 12 + (info->blocksize >> 2)) {

		if (!inode->sibptr) {

			inode->sibptr = ext2_allocate_block(info);
			if (!inode->sibptr) return 0;

			update = true;
			memset(buf, 0, info->blocksize);
		}
		else ext2_read_block(info, inode->sibptr, buf);

		/* get block from table */
		block = ((uint32_t *)buf)[idx-12];
		if (!block) {

			block = ext2_allocate_block(info);
			if (!block) return 0;

			((uint32_t *)buf)[idx-12] = block;
			ext2_write_block(info, inode->sibptr, buf);

			inode->nsectors += (info->blocksize >> 9);
			update = true;
		}

		ext2_read_block(info, block, buf);
	}

	if (update) ext2_write_inode(info, ninode, inode);
	return block;
}

/* read from file */
static kssize_t ext2_read(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	struct ext2_fs_info *info = (struct ext2_fs_info *)node->data;
	struct ext2_file_info *file = (struct ext2_file_info *)node->odata;

	info->held = true;

	/* copy bytes */
	size_t count;
	for (count = 0; count < nbytes; count++) {

		size_t pos = offset + count;
		if (pos >= node->len) return count;

		/* read next block */
		if (!file->bblk || (pos / info->blocksize) != file->bidx) {

			file->bidx = pos / info->blocksize;

			if (!file->bdata) file->bdata = kmalloc(info->blocksize);
			file->bblk = ext2_read_inode_block(info, &file->inode, file->bidx, file->bdata);

			if (!file->bblk) return count;
		}

		buf[count] = ((uint8_t *)file->bdata)[pos % info->blocksize];
	}

	info->held = false;
	return count;
}

/* write to file */
static kssize_t ext2_write(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	struct ext2_fs_info *info = (struct ext2_fs_info *)node->data;
	struct ext2_file_info *file = (struct ext2_file_info *)node->odata;

	info->held = true;

	/* copy bytes */
	size_t count = 0, pos = 0;
	for (; count < nbytes; count++) {

		pos = offset + count;

		/* write previous block and read next block */
		if (!file->bblk || (pos / info->blocksize) != file->bidx) {

			if (file->bblk)
				ext2_write_block(info, file->bblk, file->bdata);

			file->bidx = pos / info->blocksize;

			if (!file->bdata) file->bdata = kmalloc(info->blocksize);
			file->bblk = ext2_set_inode_block(info, node->inode, &file->inode, file->bidx, file->bdata);

			if (!file->bblk) break;
		}

		((uint8_t *)file->bdata)[pos % info->blocksize] = buf[count];
	}
	if (count && file->bblk) ext2_write_block(info, file->bblk, file->bdata);

	/* update file length/size */
	if (pos+1 > node->len) {

		node->len = pos+1;
		file->inode.losize = (uint32_t)node->len;
		ext2_write_inode(info, node->inode, &file->inode);
	}

	info->held = false;
	return count;
}

/* open file */
static void ext2_open(fs_node_t *node, uint32_t flags) {

	struct ext2_fs_info *info = (struct ext2_fs_info *)node->data;
	info->held = true;

	node->odata = kmalloc(sizeof(struct ext2_file_info));
	struct ext2_file_info *file = (struct ext2_file_info *)node->odata;

	file->bblk = 0;
	file->bidx = 0;
	file->bdata = NULL;
	file->bpdata = NULL;
	ext2_read_inode((struct ext2_fs_info *)node->data, node->inode, &file->inode);

	info->held = false;
}

/* close file */
static void ext2_close(fs_node_t *node) {

	struct ext2_fs_info *info = (struct ext2_fs_info *)node->data;
	info->held = true;

	struct ext2_file_info *file = (struct ext2_file_info *)node->odata;
	if (file->bdata) kfree(file->bdata);
	if (file->bpdata) kfree(file->bpdata);

	info->held = false;
}

/* filldir */
static bool ext2_filldir(fs_node_t *node) {

	if (!node || node->first) return false; /* directory has already been filled */

	struct ext2_fs_info *info = node->data;
	info->held = true;

	/* read directory inode */
	ext2_inode_t *dir_inode = (ext2_inode_t *)kmalloc(sizeof(ext2_inode_t));
	ext2_read_inode(info, node->inode, dir_inode);

	/* read file data */
	uint32_t pos = 0;
	uint32_t bidx = 0; /* block index */
	ext2_inode_t *dirent_inode = (ext2_inode_t *)kmalloc(sizeof(ext2_inode_t));
	void *dirbuf = kmalloc(info->blocksize);
	bool found = true; /* found block */

	while (pos < dir_inode->losize) {

		/* read next block */
		uint32_t lpos = pos % info->blocksize;
		if (!lpos && !ext2_read_inode_block(info, dir_inode, bidx++, dirbuf))
			break;

		/* add directory entry */
		ext2_dirent_t *fsdent = (ext2_dirent_t *)(dirbuf + lpos);
		size_t dnamesz = fsdent->lonamelen > FS_NAMESZ? FS_NAMESZ: fsdent->lonamelen;
		
		fs_dirent_t *dent = fs_dirent_new(NULL);
		strncpy(dent->name, fsdent->name, dnamesz);

		/* create node */
		ext2_read_inode(info, fsdent->inode, dirent_inode);
		uint32_t flags = ext2_translate_type(dirent_inode->type);

		fs_node_t *child = fs_node_new(node, flags);
		child->mask = 0; /* todo */
		child->uid = dirent_inode->uid;
		child->gid = dirent_inode->gid;
		child->inode = fsdent->inode;
		child->len = dirent_inode->losize;
		dent->node = child;

		fs_node_add_dirent(node, dent);
		pos += fsdent->entsize;
	}

	kfree(dir_inode);
	kfree(dirent_inode);
	kfree(dirbuf);

	info->held = false;
	return true;
}

/* check if resource is busy */
extern bool ext2_isheld(fs_node_t *node) {

	struct ext2_fs_info *info = node->data;
	return node->held || info->held || info->dev->held;
}

/* mount ext2 filesystem */
extern fs_node_t *ext2_mbr_mount(fs_node_t *mountp, device_t *dev, mbr_ent_t *part) {

	if (mountp->ptr) return NULL;

	struct ext2_fs_info *info = kmalloc(sizeof(struct ext2_fs_info));

	/* read superblock */
	device_storage_read(dev, part->start_lba + 2, 2, info->sbpad);
	if (!ext2_verify_sb(&info->sb)) {

		kfree(info);
		kprintf(LOG_WARNING, "Invalid Ext2 superblock");
		return NULL;
	}

	info->blocksize = 1024 << info->sb.blocksize;
	info->inodesize = info->sb.vmajor >= 1? info->sb.ext_inodesize: 128;
	info->dev = dev;
	info->part = part;
	info->curblock = 0;
	info->block = NULL;

	/* load block group descriptor table */
	ext2_load_bgdt(info);

	/* create root node */
	fs_node_t *node = fs_node_new(NULL, FS_DIRECTORY);
	node->data = info;
	node->inode = 2;
	node->parent = mountp->parent;

	/* operations */
	node->read = ext2_read;
	//node->write = ext2_write;
	node->open = ext2_open;
	node->close = ext2_close;
	node->filldir = ext2_filldir;
	node->isheld = ext2_isheld;
	
	mountp->ptr = node;
	return node;
}
