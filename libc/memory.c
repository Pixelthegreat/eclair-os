#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <ec.h>

/* memory block */
struct hblock {
	size_t size; /* size of block */
	bool avail; /* is available */
	struct hblock *prev; /* previous block */
	struct hblock *next; /* next block */
};
static struct hblock *first = NULL;

/* initialize heap */
extern int __libc_init_heap(void) {

	first = (struct hblock *)ec_sbrk(sizeof(struct hblock));
	if (!first) return -1;

	first->size = 0;
	first->avail = false;
	first->prev = NULL;
	first->next = NULL;

	return 0;
}

/* finalize heap */
extern void __libc_fini_heap(void) {
}

/* find an unused block */
static struct hblock *find_block(struct hblock *head, size_t size) {

	struct hblock *p = head;
	struct hblock *l = NULL;
	while (p) {

		if ((p->size >= size || !p->next) && p->avail)
			return p;

		l = p;
		p = p->next;
	}

	/* set next block */
	l->next = (struct hblock *)ec_sbrk(sizeof(struct hblock) + (intptr_t)size);
	if (!l->next) return NULL;

	l->next->size = 0;
	l->next->avail = true;
	l->next->prev = l;
	l->next->next = NULL;

	return l->next;
}

/* split block */
static void split_block(struct hblock *block, size_t size) {

	if (block->size < size + sizeof(struct hblock)) return;

	struct hblock *next = (struct hblock *)((void *)(block + 1) + size);
	
	next->size = block->size - size - sizeof(struct hblock);
	next->avail = true;
	next->next = block->next;
	next->prev = block;

	next->size = size;
	block->next = next;

	if (next->next) next->next->prev = next;
}

/* merge with previous block */
static struct hblock *merge_block_back(struct hblock *block) {

	while (block->prev && block->prev->avail) {

		block->prev->size += sizeof(struct hblock) + block->size;

		block->prev->next = block->next;
		if (block->next) block->next->prev = block->prev;
		block = block->prev;
	}
	return block;
}

/* merge with next block */
static void merge_block_front(struct hblock *block) {

	while (block->next && block->next->avail &&
	       (block->next->size > 0 || !block->next->next)) {

		block->size += sizeof(struct hblock) + block->next->size;

		block->next = block->next->next;
		if (block->next) block->next->prev = block;
	}
}

/* merge block with previous and next */
static struct hblock *merge_block(struct hblock *block) {

	block = merge_block_back(block);
	merge_block_front(block);

	return block;
}

/* memory allocation functions */
extern void free(void *ptr) {

	struct hblock *block = (struct hblock *)ptr - 1;

	block->avail = true;
	block = merge_block(block);

	if (!block->next) {

		if (block->prev) block->prev->next = NULL;
		ec_sbrk(-(sizeof(struct hblock) + (intptr_t)block->size));
	}
}

extern void *malloc(size_t size) {

	size = EC_ALIGN(size, 4);

	struct hblock *block = find_block(first, size);
	if (!block) return NULL;

	block->avail = false;

	split_block(block, size);
	block->size = size;

	return (void *)(block + 1);
}
