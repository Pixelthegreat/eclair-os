/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This is primarily just a stub header for getopt.
 */
#ifndef _UNISTD_H
#define _UNISTD_H 1

extern int getopt(int argc, const char *const *argv, const char *optstring);

extern const char *optarg;
extern int optind;

#endif /* _UNISTD_H */
