/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/heap.h>

static heap_block_t *head = NULL;
static heap_block_t *last = NULL;

/* initialize heap */
extern void heap_init(void) {

	page_frame_id_t fr = page_frame_alloc();
	page_id_t pg = page_breakp;
	page_map(page_breakp, fr);

	/* setup head block */
	head = PAGE_ADDR(pg);
	head->size = 0;
	head->avail = false;
	head->page = pg;
	head->prev = NULL;
	head->next = NULL;
}

/* find unused block */
extern heap_block_t *heap_find(heap_block_t *b, size_t sz) {

	heap_block_t *p = b;
	heap_block_t *l = NULL;
	while (p) {

		/* usable */
		if ((p->size >= sz || !p->next) && p->avail)
			return p;

		l = p;
		p = p->next;
	}
	
	/* set next block */
	l->next = (void *)(l + 1) + l->size;
	l->next->size = 0;
	l->next->avail = true;
	l->next->page = (uint32_t)l->next >> 12;
	l->next->prev = l;
	l->next->next = NULL;
	return l->next;
}

/* find an aligned block */
extern heap_block_t *heap_find_aligned(heap_block_t *b, size_t sz, size_t a) {

	heap_block_t *p = b;
	heap_block_t *l = NULL;
	while (p) {

		/* usable */
		if ((p->size >= sz || !p->next) && p->avail &&\
		    !(((size_t)p + sizeof(heap_block_t)) & (a-1)))
			return p;

		l = p;
		p = p->next;
	}

	size_t pl = (size_t)(l + 1);
	pl = ALIGN(pl + l->size + sizeof(heap_block_t), a) - pl - sizeof(heap_block_t);

	/* set next block */
	l->next = (heap_block_t *)((void *)(l + 1) + pl);
	l->next->size = 0;
	l->next->avail = true;
	l->next->page = (uint32_t)l->next >> 12;
	l->next->prev = l;
	l->next->next = NULL;

	heap_block_t *bp = l->next;

	/* insert block */
	size_t nsz = pl - l->size;

	if (l->avail) l->size = pl;
	else if (nsz >= sizeof(heap_block_t)) {

		heap_block_t *n = (heap_block_t *)((void *)(l + 1) + l->size);
		n->size = nsz - sizeof(heap_block_t);
		n->avail = true;
		n->page = (uint32_t)n >> 12;
		n->next = l->next;
		n->prev = l;
		l->next = n;

		if (n->next) n->next->prev = n;
	}

	return bp;
}

/* split block */
extern void heap_split(heap_block_t *b, size_t sz) {

	if (b->size < sz + sizeof(heap_block_t)) return;

	/* set the next block */
	heap_block_t *n = (heap_block_t *)((void *)(b + 1) + sz);
	n->size = b->size - sz - sizeof(heap_block_t);
	n->avail = true;
	n->page = (uint32_t)n >> 12;
	n->next = b->next;
	n->prev = b;

	b->size = sz;
	b->next = n;

	if (n->next) n->next->prev = n;
}

/* merge with previous block */
static heap_block_t *heap_merge_back(heap_block_t *b) {

	while (b->prev && b->prev->avail) {

		b->prev->size += sizeof(heap_block_t) + b->size;
		b->prev->next = b->next;
		if (b->next) b->next->prev = b->prev;
		b = b->prev;
	}
	return b;
}

/* merge with next block */
static void heap_merge_front(heap_block_t *b) {

	while (b->next && b->next->avail && (b->next->size > 0 || !b->next->next)) {

		b->size += sizeof(heap_block_t) + b->next->size;
		b->next = b->next->next;
		if (b->next) b->next->prev = b;
	}
}

/* merge block */
extern heap_block_t *heap_merge(heap_block_t *b) {

	b = heap_merge_back(b);
	heap_merge_front(b);
	return b;
}

/* debug */
extern void heap_print(void) {

	heap_block_t *b = head;
	while (b != NULL) {

		tty_printf("addr: 0x%x, available: %d, size: %d\n", b, b->avail, b->size);
		b = b->next;
	}
}

/* allocate */
extern void *kmalloc(size_t sz) {

	sz = ALIGN(sz, 4);

	heap_block_t *b = heap_find(head, sz);
	if (!b) return NULL;

	/* map pages */
	uint32_t boff = (uint32_t)b - b->page * 4096;
	uint32_t pagel = (boff + sz + (sizeof(heap_block_t) * 2)) / 4096 + 1;
	
	for (uint32_t i = b->page; i < b->page + pagel; i++) {
		if (!page_is_mapped(i))
			page_map(i, page_frame_alloc());
	}

	/* set values */
	b->avail = false;

	/* split block if possible */
	heap_split(b, sz);
	b->size = sz;

	/* return data pointer */
	last = b;
	return (void *)(b + 1);
}

/* allocate sz bytes aligned to a bytes */
extern void *kmalloca(size_t sz, size_t a) {

	sz = ALIGN(sz, 4);

	heap_block_t *b = heap_find_aligned(head, sz, a);
	if (!b) return NULL;

	/* map pages */
	uint32_t boff = (uint32_t)b - b->page * 4096;
	uint32_t pagel = (boff + sz + (sizeof(heap_block_t) * 2)) / 4096 + 1;
	
	for (uint32_t i = b->page; i < b->page + pagel; i++) {
		if (!page_is_mapped(i))
			page_map(i, page_frame_alloc());
	}

	/* set values */
	b->avail = false;

	/* split block if possible */
	heap_split(b, sz);
	b->size = sz;

	/* return data pointer */
	last = b;
	return (void *)(b + 1);
}

/* free */
extern void kfree(void *p) {

	if (p == NULL) return;
	heap_block_t *b = (heap_block_t *)p - 1;

	/* set values */
	b->avail = true;

	/* merge blocks */
	b = heap_merge(b);

	/* unmap pages */
	page_id_t pg = b->page + 1;
	while (((pg + 1) * 4096) < (uint32_t)b->next) {

		page_frame_free(page_get_frame(pg));
		page_unmap(pg);
		pg++;
	}
}
