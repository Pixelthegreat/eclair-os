/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_DRIVER_FBCON_H
#define ECLAIR_DRIVER_FBCON_H

#include <kernel/types.h>

/* functions */
extern void fbcon_init(void); /* initialize */
extern void fbcon_init_tty(void); /* initialize tty device */
extern kssize_t fbcon_read(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf); /* read */
extern kssize_t fbcon_write(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf); /* write */
extern int fbcon_ioctl(fs_node_t *_dev, int op, uintptr_t arg); /* io control */
extern void fbcon_flip_cursor(void); /* show/hide cursor */
extern void fbcon_update(void); /* update console */

#endif /* ECLAIR_DRIVER_FBCON_H */
