/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _CTYPE_H
#define _CTYPE_H 1

#define tolower(c) (((c) >= 'A' && (c) <= 'Z')? (c)+32: (c))
#define toupper(c) (((c) >= 'a' && (c) <= 'z')? (c)-32: (c))

#endif /* _CTYPE_H */
