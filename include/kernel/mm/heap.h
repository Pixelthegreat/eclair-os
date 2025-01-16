#ifndef ECLAIR_HEAP_H
#define ECLAIR_HEAP_H

#include <kernel/types.h>
#include <kernel/mm/paging.h>

typedef struct heap_block {
	size_t size; /* size of block */
	bool avail; /* available */
	page_id_t page; /* page that the beginning of the block lies on */
	struct heap_block *prev; /* previous */
	struct heap_block *next; /* next */
} heap_block_t;

/* functions */
extern void heap_init(void); /* initialize heap */
extern heap_block_t *heap_find(heap_block_t *b, size_t sz); /* find a block */
extern void heap_split(heap_block_t *b, size_t sz); /* split block into two */
extern heap_block_t *heap_merge(heap_block_t *b); /* merge chunk with front and back */
extern void heap_print(void); /* debug */

extern void *kmalloc(size_t sz); /* allocate sz bytes */
extern void kfree(void *p); /* free pointer */

#endif /* ECLAIR_HEAP_H */
