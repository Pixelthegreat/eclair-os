/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WM_SERVER_H
#define WM_SERVER_H

#include <stdint.h>

/* server resource */
typedef struct resource {
	uint32_t id; /* resource id */
	uint32_t type; /* resource type */
	int refcnt; /* reference count */
	int owner; /* owning process */
	void *data; /* user data */
} resource_t;

/* functions */
extern int server_host(void); /* host server */
extern resource_t *server_create_resource(uint32_t type); /* allocate resource */
extern resource_t *server_get_resource(uint32_t id); /* get resource by id */
extern void server_destroy_resource(resource_t *resource); /* deallocate resource */
extern int server_update(void); /* update server */

#endif /* WM_SERVER_H */
