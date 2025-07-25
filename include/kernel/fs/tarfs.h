/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_FS_TARFS_H
#define ECLAIR_FS_TARFS_H

#include <kernel/types.h>
#include <kernel/vfs/fs.h>

/* functions */
extern fs_node_t *tarfs_mount(fs_node_t *mountp, void *addr); /* mount filesystem */

#endif /* ECLAIR_FS_TARFS_H */
