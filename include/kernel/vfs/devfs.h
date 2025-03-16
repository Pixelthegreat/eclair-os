#ifndef ECLAIR_VFS_DEVFS_H
#define ECLAIR_VFS_DEVFS_H

#include <kernel/types.h>
#include <kernel/vfs/fs.h>

#define DEVFS_DIR "/dev"

/* functions */
extern void devfs_init(void); /* initialize */
extern void devfs_add_node(const char *prefix, fs_node_t *node); /* add node to /dev */
extern void devfs_add_unique(const char *name, fs_node_t *node); /* add node with unique name */

#endif /* ECLAIR_VFS_DEVFS_H */
