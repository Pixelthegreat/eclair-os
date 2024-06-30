#include <e.clair/types.h>
#include <e.clair/tty.h>
#include <e.clair/mm/paging.h>
#include <e.clair/heap.h>

static heap_block_t *head = NULL;

/* initialize heap */
extern void heap_init(void) {

	page_frame_id_t fr = page_frame_alloc();
	page_id_t pg = page_alloc(1, &fr);

	/* setup head block */
	head = PAGE_ADDR(pg);
	head->size = 0;
	head->avail = false;
	head->page = pg;
	head->prev = NULL;
	head->next = head + 1;

	/* next */
	head->next->size = 0;
	head->next->avail = true;
	head->next->page = pg;
	head->next->prev = head;
	head->next->next = NULL;
}

/* find unused block */
extern heap_block_t *heap_find(heap_block_t *b, size_t sz) {

	heap_block_t *p = b;
	while (p != NULL) {

		/* usable */
		if ((p->size >= sz || p->next == NULL) && p->avail)
			return p;

		p = p->next;
	}
	return NULL;
}

/* split block */
extern void heap_split(heap_block_t *b, size_t sz) {

	if (!b->next || b->size < sz + sizeof(heap_block_t)) return;

	/* set the next block */
	heap_block_t *n = (heap_block_t *)((void *)(b + 1) + sz);
	n->size = b->size - sz - sizeof(heap_block_t);
	n->avail = true;
	n->page = (uint32_t)n / 4096;
	n->next = b->next;
	n->prev = b;

	b->size = sz;
	b->next = n;
}

/* merge with previous block */
static heap_block_t *heap_merge_back(heap_block_t *b) {

	if (!b->prev || !b->prev->avail) return b;

	b->prev->size += sizeof(heap_block_t) + b->size;
	b->prev->next = b->next;
	return b->prev;
}

/* merge with next block */
static void heap_merge_front(heap_block_t *b) {

	if (!b->next || !b->next->avail) return;

	b->size += sizeof(heap_block_t) + b->next->size;
	b->next = b->next->next;
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

	heap_block_t *b = heap_find(head, sz);
	if (b == NULL) return NULL;

	/* map pages */
	uint32_t boff = (uint32_t)b - b->page * 4096;
	uint32_t pagel = (boff + (b->next? b->size: sz) + (sizeof(heap_block_t) * 2)) / 4096 + 1;
	for (uint32_t i = b->page; i < b->page + pagel; i++) {

		if (!page_is_mapped(i))
			page_map(i, page_frame_alloc());
	}

	/* set values */
	b->avail = false;
	if (!b->size) b->size = sz;

	/* split block if possible */
	heap_split(b, sz);

	/* set values for next */
	if (!b->next) {

		b->next = (void *)(b + 1) + sz;
		b->next->size = 0;
		b->next->avail = true;
		b->next->page = (uint32_t)b->next / 4096;
		b->next->prev = b;
		b->next->next = NULL;
	}

	/* return data pointer */
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
	while ((pg * 4096 + 4096) < (uint32_t)b->next) {

		page_frame_free(page_get_frame(pg));
		page_unmap(pg);
		pg++;
	}
}
