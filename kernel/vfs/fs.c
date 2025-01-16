#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/tty.h>
#include <kernel/mm/heap.h>
#include <kernel/vfs/fs.h>

fs_node_t *fs_root; /* root node */

/* initialize vfs */
extern void fs_init(void) {

	fs_root = fs_node_new(NULL, FS_MOUNTPOINT);
}

/* create new directory entry */
extern fs_dirent_t *fs_dirent_new(const char *name) {

	fs_dirent_t *dent = (fs_dirent_t *)kmalloc(sizeof(fs_dirent_t));

	if (name) strncpy(dent->name, name, FS_NAMESZ);
	else dent->name[0] = 0;
	dent->node = NULL;
	dent->prev = NULL;
	dent->next = NULL;

	return dent;
}

/* create new node */
extern fs_node_t *fs_node_new(fs_node_t *parent, uint32_t flags) {

	fs_node_t *node = (fs_node_t *)kmalloc(sizeof(fs_node_t));
	memset(node, 0, sizeof(fs_node_t));
	node->flags = flags;

	/* copy file operations and fs specific stuff */
	if (parent) {

		node->data = parent->data;
		node->mask = parent->mask;
		node->uid = parent->uid;
		node->gid = parent->gid;
		node->impl = parent->impl;

		node->read = parent->read;
		node->write = parent->write;
		node->open = parent->open;
		node->close = parent->close;
		node->filldir = parent->filldir;

		node->parent = parent;
	}

	return node;
}

/* add dirent */
extern void fs_node_add_dirent(fs_node_t *node, fs_dirent_t *dent) {

	while (node->ptr) node = node->ptr;

	/* set dirent values */
	dent->prev = node->last;
	dent->next = NULL;

	if (node->last) node->last->next = dent;
	if (!node->first) node->first = dent;
	node->last = dent;
}

/* print node tree */
static int level = 0;
extern void fs_node_print(fs_node_t *node) {

	if (!node) return;

	/* print dirents */
	fs_dirent_t *dent = node->first;
	while (dent) {

		for (int i = 0; i < level; i++) tty_printf("   ");
		tty_printf("%s\n", dent->name);
		level++;
		fs_node_print(dent->node);
		level--;

		dent = dent->next;
	}
}

/* read from file */
extern kssize_t fs_read(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	if (!node->refcnt || !(node->oflags & FS_READ) || !node->read) return 0;
	return node->read(node, offset, nbytes, buf);
}

/* write to file */
extern kssize_t fs_write(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	if (!node->refcnt || !(node->oflags & FS_WRITE) || !node->write) return 0;
	return node->write(node, offset, nbytes, buf);
}

/* open file */
extern void fs_open(fs_node_t *node, uint32_t flags) {

	if (node->refcnt && node->oflags != flags) return;

	if (node->open) node->open(node, flags);
	node->oflags = flags;
	node->refcnt++;
}

/* close file */
extern void fs_close(fs_node_t *node) {

	if (!node->refcnt) return;

	if (node->close) node->close(node);
	node->refcnt--;
}

/* read directory entry */
extern fs_dirent_t *fs_readdir(fs_node_t *node, uint32_t idx) {

	if (!node) return NULL;
	while (node->ptr) node = node->ptr;

	/* fill directory entries (should have . and ..) */
	if (!node->first) node->filldir(node);

	/* dirents */
	fs_dirent_t *dent = node->first;
	uint32_t i = 0;
	while (dent) {

		if (i == idx) return dent;
		dent = dent->next;
		i++;
	}
	return NULL;
}

/* find in directory */
extern fs_node_t *fs_finddir(fs_node_t *node, const char *name) {

	if (!node || !(node->flags & FS_DIRECTORY)) return NULL;

	fs_dirent_t *dent = fs_readdir(node, 0);
	while (dent != NULL) {

		if (!strcmp(dent->name, name))
			return dent->node;
		dent = dent->next;
	}
	return NULL;
}
