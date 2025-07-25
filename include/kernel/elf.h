/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_ELF_H
#define ECLAIR_ELF_H

#include <kernel/types.h>

/* functions */
extern int elf_load_task(const char *path, const char **argv, const char **envp, bool freeargs); /* load an executable */

#endif /* ECLAIR_ELF_H */
