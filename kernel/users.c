/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/mm/heap.h>
#include <kernel/vfs/fs.h>
#include <kernel/users.h>

static char *users_text = NULL;
static char *groups_text = NULL;

#define MAX_USERS 1024
static user_t *users[MAX_USERS];
static user_t *ufirst = NULL;
static user_t *ulast = NULL;

#define MAX_GROUPS 1024
static group_t *groups[MAX_GROUPS];
static group_t *gfirst = NULL;
static group_t *glast = NULL;

/* converters */
static int to_int(const char *s) {

	int i = 0;
	while (*s) {

		char c = *s++;
		if (c >= '0' && c <= '9')
			i = i * 10 + ((int)c - '0');
	}
	return i;
}

static uint32_t to_hex(const char *s) {

	uint32_t x = 0;
	while (*s) {

		char c = *s++;
		if (c >= '0' && c <= '9')
			x = (x << 4) | (uint32_t)(c - '0');
		else if (c >= 'a' && c <= 'f')
			x = (x << 4) | (uint32_t)(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			x = (x << 4) | (uint32_t)(c - 'A' + 10);
	}
	return x;
}

/* parse file text */
#define USERS 1
#define GROUPS 2

#define ELEM_SIZE 32
static char elem1[ELEM_SIZE], elem2[ELEM_SIZE], elem3[ELEM_SIZE];

static const char *read_elem(const char *text, char *elem) {

	if (!text) kpanic(PANIC_CODE_NONE, "Invalid user/database: Expected element", NULL);

	const char *end = strchrs(text, ":\n");
	size_t len = end? (size_t)(end - text): strlen(text);
	if (end && *end == ':') end++;

	strncpy(elem, text, len);
	return end;
}

static void parse_text(int db, const char *text) {

	while (text && *text) {

		/* read user info */
		if (db == USERS) {

			text = read_elem(text, elem1);
			int uid = to_int(elem1);

			text = read_elem(text, elem1);
			text = read_elem(text, elem2);

			text = read_elem(text, elem3);
			uint32_t pswd = to_hex(elem3);

			group_t *cur = gfirst;
			while (cur) {

				if (!strcmp(cur->name, elem2))
					break;
				cur = cur->next;
			}
			if (!cur) kpanic(PANIC_CODE_NONE, "Invalid user database: Unknown group", NULL);

			(void)user_register(uid, cur->id, elem1, pswd);
		}

		/* read group info */
		else if (db == GROUPS) {

			text = read_elem(text, elem1);
			int gid = to_int(elem1);

			text = read_elem(text, elem1);

			(void)user_register_group(gid, elem1);
		}

		/* read newline */
		if (text) {
			if (*text != '\n') kpanic(PANIC_CODE_NONE, "Invalid user/group database: Empty line", NULL);
			text++;
		}
	}
}

/* register user */
extern user_t *user_register(int uid, int gid, const char *name, uint32_t pswd) {

	if (uid < 0 || uid >= MAX_USERS || gid < 0 || gid >= MAX_GROUPS ||
	    users[uid] || !groups[gid])
		return NULL;

	users[uid] = (user_t *)kmalloc(sizeof(user_t));

	users[uid]->uid = uid;
	users[uid]->gid = gid;
	strncpy(users[uid]->name, name, USER_NAMESZ);
	users[uid]->pswd = pswd;

	users[uid]->next = NULL;
	if (!ufirst) ufirst = users[uid];
	if (ulast) ulast->next = users[uid];
	ulast = users[uid];

	return users[uid];
}

/* register group */
extern group_t *user_register_group(int gid, const char *name) {

	if (gid < 0 || gid >= MAX_GROUPS || groups[gid])
		return NULL;

	groups[gid] = (group_t *)kmalloc(sizeof(group_t));
	groups[gid]->id = gid;
	strncpy(groups[gid]->name, name, USER_NAMESZ);

	groups[gid]->next = NULL;
	if (!gfirst) gfirst = groups[gid];
	if (glast) glast->next = groups[gid];
	glast = groups[gid];

	return groups[gid];
}

/* get user info for id */
extern user_t *user_get(int uid) {

	if (uid < 0 || uid >= MAX_USERS)
		return NULL;
	return users[uid];
}

/* get group info for id */
extern group_t *user_get_group(int gid) {

	if (gid < 0 || gid >= MAX_GROUPS)
		return NULL;
	return groups[gid];
}

/* initialize user database */
extern void user_init(void) {

	fs_node_t *nusers = fs_resolve("/etc/users");
	if (!users) kpanic(PANIC_CODE_NONE, "Failed to locate user database", NULL);

	fs_node_t *ngroups = fs_resolve("/etc/groups");
	if (!groups) kpanic(PANIC_CODE_NONE, "Failed to locate group database", NULL);

	/* read files */
	fs_open(nusers, FS_READ);

	users_text = (char *)kmalloc((size_t)nusers->len+1);
	fs_read(nusers, 0, (size_t)nusers->len, users_text);
	users_text[nusers->len] = 0;

	fs_close(nusers);
	fs_open(ngroups, FS_READ);

	groups_text = (char *)kmalloc((size_t)ngroups->len+1);
	fs_read(ngroups, 0, (size_t)ngroups->len, groups_text);
	groups_text[ngroups->len] = 0;

	fs_close(ngroups);

	/* parse files */
	parse_text(GROUPS, groups_text);
	parse_text(USERS, users_text);

	kprintf(LOG_INFO, "[users] Loaded user and group databases");
}
