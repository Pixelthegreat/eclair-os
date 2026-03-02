/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * STB inspired hashmap implementation
 */
#ifndef EC_HMAP_H
#define EC_HMAP_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Raise value to power
 */
static inline uint32_t ec_hmap_hashtpow(uint32_t x, uint32_t y) {

	uint32_t r = 1;
	for (uint32_t i = 0; i < y; i++)
		r *= x;
	return r;
}

/*
 * Hash a string
 */
static inline uint32_t ec_hmap_strhash(const char *s) {

	if (!*s) return 0;

	uint32_t res = 0;
	uint32_t len = (uint32_t)strlen(s);

	for (uint32_t i = 0; i < len-1; i++)
		res += (uint32_t)(*s++) * ec_hmap_hashtpow(31, len-i-1);
	res += (uint32_t)(*s++);

	return res;
}

/*
 * Define a hashmap type
 *
 * A hashmap type assumes the associated
 * 'ec_hmap_<name>_set function' exists, in order to
 * set item values within a hashmap. For example:
 *   void ec_hmap_int_set(void *ent, const void *val);
 */
struct ec_hmap {
	size_t entsz;
	void (*setter)(struct ec_hmap *ent, uintptr_t val);
	uint32_t key;
	struct ec_hmap *next;
};

#define ec_hmap_define_type(name, ...) struct ec_hmap_##name {\
	size_t entsz; /* size of hashmap entry */\
	void (*setter)(struct ec_hmap_##name *ent, uintptr_t val); /* value setter */\
	uint32_t key; /* hashed key */\
	struct ec_hmap_##name *next; /* next entry */\
	__VA_ARGS__; /* value */\
}

#define ec_hmap_type(name) struct ec_hmap_##name

/*
 * Initialize hashmap
 *
 * Example:
 *   ec_hmap_type(int) hmap;
 *   ec_hmap_init(hmap, int);
 */
#define ec_hmap_init(hmap, type) ({\
		(hmap) = (struct ec_hmap_##type *)malloc(sizeof(struct ec_hmap_##type));\
		(hmap)->entsz = sizeof(struct ec_hmap_##type);\
		(hmap)->setter = (void (*)(struct ec_hmap_##type *, uintptr_t))ec_hmap_##type##_set;\
		(hmap)->key = 0;\
		(hmap)->next = NULL;\
	})\

/*
 * Set item in hashmap
 */
static inline void _ec_hmap_set(void *hmap, const char *key, uintptr_t val) {

	struct ec_hmap *p_hmap = (struct ec_hmap *)hmap;

	uint32_t hkey = ec_hmap_strhash(key);

	struct ec_hmap *prev = p_hmap, *cur = p_hmap->next;
	for (; cur; prev = cur, cur = cur->next) {
		if (cur->key == hkey) break;
	}
	if (!cur) {

		cur = (struct ec_hmap *)malloc(p_hmap->entsz);
		memcpy(cur, p_hmap, p_hmap->entsz);
		cur->key = hkey;
		prev->next = cur;
	}
	cur->setter(cur, val);
}

#define ec_hmap_set(hmap, key, val) _ec_hmap_set((hmap), (key), (uintptr_t)(val))

/*
 * Get item in hashmap
 */
static inline void *ec_hmap_get(void *hmap, const char *key) {

	uint32_t hkey = ec_hmap_strhash(key);

	struct ec_hmap *cur = ((struct ec_hmap *)hmap)->next;
	for (; cur; cur = cur->next) {
		if (cur->key == hkey)
			return cur;
	}
	return NULL;
}

/*
 * Free hashmap
 */
static inline void ec_hmap_free(void *hmap) {

	if (!hmap) return;

	struct ec_hmap *cur = (struct ec_hmap *)hmap;
	struct ec_hmap *next = cur->next;

	for (; cur; cur = next, next = cur? cur->next: cur)
		free(cur);
}

#endif /* EC_HMAP_H */
