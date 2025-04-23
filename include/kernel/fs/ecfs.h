#ifndef ECLAIR_FS_ECFS_H
#define ECLAIR_FS_ECFS_H

#include <kernel/types.h>
#include <kernel/driver/device.h>
#include <kernel/vfs/fs.h>
#include <kernel/fs/mbr.h>

/* functions */
extern fs_node_t *ecfs_mbr_mount(fs_node_t *mountp, device_t *dev, mbr_ent_t *part);

#endif /* ECLAIR_FS_ECFS_H */
