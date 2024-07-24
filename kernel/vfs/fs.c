#include <e.clair/types.h>
#include <e.clair/string.h>
#include <e.clair/tty.h>
#include <e.clair/mm/heap.h>
#include <e.clair/vfs/fs.h>

fs_node_t *fs_root; /* root node */

/* initialize vfs */
extern void fs_init(void) {

	fs_root = fs_node_new(NULL, FS_MOUNTPOINT);
}

/* create new directory entry */
extern fs_dirent_t *fs_dirent_new(const char *name) {

	fs_dirent_t *dent = (fs_dirent_t *)kmalloc(sizeof(fs_dirent_t));

	strncpy(dent->name, name, FS_NAMESZ);
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
		node->impl = parent->impl;

		node->read = parent->read;
		node->write = parent->write;
		node->open = parent->open;
		node->close = parent->close;
		node->readdir = parent->readdir;
		node->finddir = parent->finddir;
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
extern ssize_t fs_read(fs_node_t *node, off_t offset, size_t nbytes, uint8_t *buf) {

	if (!node->refcnt || !(node->flags & FS_READ) || !node->read) return 0;
	return node->read(node, offset, nbytes, buf);
}

/* write to file */
extern ssize_t fs_write(fs_node_t *node, off_t offset, size_t nbytes, uint8_t *buf) {

	if (!node->refcnt || !(node->flags & FS_WRITE) || node->write) return 0;
	return node->read(node, offset, nbytes, buf);
}

/* open file */
extern void fs_open(fs_node_t *node, uint32_t flags) {

	if (node->refcnt && node->flags != flags) return;

	if (node->open) node->open(node, flags);
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

	fs_dirent_t *dent = node->first;
	uint32_t i = 0;
	while (dent) {

		if (i == idx) return dent;
		dent = dent->next;
		i++;
	}
	
	/* fs readdir function expected to fill directory if necessary */
	if (node->readdir) return node->readdir(node, idx);
	return NULL;
}

/* find in directory */
extern fs_node_t *fs_finddir(fs_node_t *node, const char *name) {

	fs_dirent_t *dent = node->first;
	while (dent) {

		if (!strncmp(dent->name, name, FS_NAMESZ)) return dent->node;
		dent = dent->next;
	}

	/* exepected to fill directory if necessary */
	if (node->finddir) return node->finddir(node, name);
	return NULL;
}
