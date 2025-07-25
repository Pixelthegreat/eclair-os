/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Bochs Graphics Adapter stub driver for testing PCI
 */
#ifndef ECLAIR_DRIVER_BGA_H
#define ECLAIR_DRIVER_BGA_H

#include <kernel/types.h>

#define BGA_VENDORID 0x1234
#define BGA_DEVICEID 0x1111

/* functions */
extern void bga_register(void); /* register pci driver */

#endif /* ECLAIR_DRIVER_BGA_H */
