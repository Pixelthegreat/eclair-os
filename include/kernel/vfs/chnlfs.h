/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_VFS_CHNLFS_H
#define ECLAIR_VFS_CHNLFS_H

#include <kernel/types.h>
#include <kernel/vfs/fs.h>

/* functions */
extern void chnlfs_init(fs_node_t *node); /* initialize channel file system */

#endif /* ECLAIR_VFS_CHNLFS_H */
