/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_USERS_H
#define ECLAIR_USERS_H

#include <kernel/types.h>

/* user and group info */
#define USER_NAMESZ 32

typedef struct user {
	int uid; /* user identifier */
	int gid; /* group identifier */
	char name[USER_NAMESZ]; /* name */
	uint32_t pswd; /* password hash */
	struct user *next; /* next user */
} user_t;

typedef struct group {
	int id; /* group identifier */
	char name[USER_NAMESZ]; /* name */
	struct group *next; /* next group */
} group_t;

/* functions */
extern user_t *user_register(int uid, int gid, const char *name, uint32_t pswd); /* register user */
extern group_t *user_register_group(int gid, const char *name); /* register group */
extern user_t *user_get(int uid); /* get user info for id */
extern group_t *user_get_group(int gid); /* get group info for id */
extern void user_init(void); /* initialize user database */

#endif /* ECLAIR_USERS_H */
