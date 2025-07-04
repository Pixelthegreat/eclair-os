#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/driver/ps2.h>
#include <kernel/driver/uart.h>
#include <kernel/driver/ata.h>
#include <kernel/driver/fb.h>
#include <kernel/vfs/devfs.h>

static fs_node_t *dev = NULL;
static fs_node_t *null = NULL;
static fs_node_t *zero = NULL;

/* read from zero */
static kssize_t zero_read(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	memset(buf, 0, nbytes);
	return (kssize_t)nbytes;
}

/* write to zero */
static kssize_t zero_write(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	return (kssize_t)nbytes;
}

/* initialize */
extern void devfs_init(void) {

	fs_node_t *devdir = fs_resolve(DEVFS_DIR);
	if (!devdir) kpanic(PANIC_CODE_NONE, "Failed to locate '" DEVFS_DIR "'", NULL);

	while (devdir->ptr) devdir = devdir->ptr;
	devdir->flags |= FS_MOUNTPOINT;

	dev = fs_node_new(NULL, FS_DIRECTORY);
	dev->parent = devdir;

	devdir->ptr = dev;

	/* create proxy devices */
	null = fs_node_new(NULL, FS_CHARDEVICE);

	zero = fs_node_new(NULL, FS_CHARDEVICE);
	zero->read = zero_read;
	zero->write = zero_write;

	devfs_add_unique("null", null);
	devfs_add_unique("zero", zero);

	/* initialize devices */
	uart_init_devfs();
	ps2_init_devfs();
	ata_init_devfs();
	tty_init_devfs();
	if (fb_addr) fb_init_devfs();
}

/* add node to /dev */
extern void devfs_add_node(const char *prefix, fs_node_t *node) {

	size_t len = strlen(prefix);

	/* get number of devices with same prefix */
	int count = 0;
	fs_dirent_t *dent = dev->first;
	while (dent) {

		if (!strncmp(dent->name, prefix, len)) count++;
		dent = dent->next;
	}

	/* make device name */
	char buf[32];
	strncpy(buf, prefix, 31);
	if (len < 31) {
		
		buf[len] = count+'0';
		buf[len+1] = 0;
	}

	dent = fs_dirent_new(buf);
	dent->node = node;
	node->parent = dev;

	fs_node_add_dirent(dev, dent);
}

/* add node with unique name */
extern void devfs_add_unique(const char *name, fs_node_t *node) {

	fs_dirent_t *dent = fs_dirent_new(name);
	dent->node = node;
	node->parent = dev;

	fs_node_add_dirent(dev, dent);
}
