/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/boot.h>
#include <kernel/mm/heap.h>
#include <kernel/vfs/fs.h>
#include <kernel/vfs/ramfs.h>

static fs_node_t *root = NULL;

/* ramfs file info */
#define BLOCK_SIZE 1024
#define BLOCK_SHIFT 10
#define MAX_BLOCKS 64

struct ramfs_file {
	void *blocks[MAX_BLOCKS]; /* data blocks */
	size_t size; /* file size */
};

/* open ramfs file */
static void ramfs_open(fs_node_t *node, uint32_t flags) {

	struct ramfs_file *file = (struct ramfs_file *)node->data;

	if (flags & FS_TRUNCATE) {

		file->size = 0;
		node->len = 0;
	}
}

/* read from ramfs file */
static kssize_t ramfs_read(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buffer) {

	struct ramfs_file *file = (struct ramfs_file *)node->data;

	if (size > file->size - (size_t)offset)
		size = file->size - (size_t)offset;

	size_t count = 0;
	for (; count < size; count++) {

		size_t position = (size_t)offset + count;
		size_t block = position >> BLOCK_SHIFT;
		position &= (BLOCK_SIZE-1);

		if (!file->blocks[block])
			buffer[count] = 0;
		else buffer[count] = ((uint8_t *)file->blocks[block])[position];
	}
	return (kssize_t)count;
}

/* write to ramfs file */
static kssize_t ramfs_write(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buffer) {

	struct ramfs_file *file = (struct ramfs_file *)node->data;

	size_t count = 0;
	for (; count < size; count++) {

		size_t position = (size_t)offset + count;
		size_t block = position >> BLOCK_SHIFT;
		position &= (BLOCK_SIZE-1);

		if (block >= MAX_BLOCKS)
			break;
		if (!file->blocks[block]) {

			file->blocks[block] = kmalloc(BLOCK_SIZE);
			memset(file->blocks[block], 0, BLOCK_SIZE);
		}
		((uint8_t *)file->blocks[block])[position] = buffer[count];
	}
	if ((size_t)offset + count > file->size) {

		file->size = (size_t)offset + count;
		node->len = offset + (uint32_t)count;
	}
	return (kssize_t)count;
}

/* close ramfs file */
static void ramfs_close(fs_node_t *node) {

	struct ramfs_file *file = (struct ramfs_file *)node->data;

	size_t index = ALIGN(file->size, BLOCK_SIZE) >> BLOCK_SHIFT;
	for (size_t i = index; i < MAX_BLOCKS; i++) {

		if (file->blocks[i])
			kfree(file->blocks[i]);
		file->blocks[i] = NULL;
	}
}

/* stat ramfs file */
static void ramfs_stat(fs_node_t *node, ec_stat_t *st) {

	struct ramfs_file *file = (struct ramfs_file *)node->data;

	st->atime = 0;
	st->mtime = 0;
	st->ctime = 0;
	st->blksize = BLOCK_SIZE;
	st->blocks = (int)ALIGN(file->size, BLOCK_SIZE) >> BLOCK_SHIFT;
}

/* create ramfs file */
static fs_node_t *ramfs_create(fs_node_t *parent, const char *name, uint32_t flags, uint32_t mask) {

	fs_node_t *node = fs_node_new(parent, flags);
	node->mask = mask;
	node->data = kmalloc(sizeof(struct ramfs_file));
	memset(node->data, 0, sizeof(struct ramfs_file));

	fs_dirent_t *dent = fs_dirent_new(name);
	dent->node = node;

	fs_node_add_dirent(parent, dent);

	return node;
}

/* initialize */
extern void ramfs_init(void) {

	boot_cmdline_t *cmdline = boot_get_cmdline();
	if (!cmdline->ramfs_mount[0])
		return;

	fs_node_t *dir = fs_resolve(cmdline->ramfs_mount);
	if (!dir) kpanic(PANIC_CODE_NONE, "Failed to locate ramfs mountpoint", NULL);

	while (dir->ptr) dir = dir->ptr;
	dir->flags |= FS_MOUNTPOINT;

	root = fs_node_new(NULL, FS_DIRECTORY);
	root->mask = 0777;
	root->parent = dir;
	root->open = ramfs_open;
	root->read = ramfs_read;
	root->write = ramfs_write;
	root->close = ramfs_close;
	root->create = ramfs_create;
	root->stat = ramfs_stat;

	dir->ptr = root;

	kprintf(LOG_INFO, "[ramfs] Initialized ram filesystem");

	/* create dummy nodes */
	fs_node_t *dot = fs_node_new(root, FS_DIRECTORY);
	fs_dirent_t *dent = fs_dirent_new(".");
	dent->node = dot;
	dot->parent = root;
	dot->mask = 0;

	fs_node_add_dirent(root, dent);

	dot = fs_node_new(root, FS_DIRECTORY);
	dent = fs_dirent_new("..");
	dent->node = dot;
	dot->parent = root;
	dot->mask = 0;

	fs_node_add_dirent(root, dent);
}
