#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/tty.h>
#include <kernel/mm/paging.h>

/* the 0x8000-0x80000 address range (page numbers) */
#define LOWMEM_START 8
#define LOWMEM_SIZE 120

/* 4G / 4K (page size) / 8 (bits in a byte) */
#define BITMAP_SIZE 131072

extern void _kernel_end(void);

static uint8_t bitmap[BITMAP_SIZE];
static uint32_t nused; /* number of used frames */

/* page mapper */
static page_dir_entry_t *page_dir = NULL;
static page_id_t page_start = 0;
static page_id_t page_table_id = 0;

page_id_t page_breakp = 0;
page_dir_entry_t *page_dir_wrap = NULL;
page_tab_entry_t *page_table = NULL;

/* get an address to a page table from the page directory */
#define PAGE_TAB(p) ((page_tab_entry_t *)(page_dir_wrap[(p)] & 0xfffff000))

/* initialize frame allocator */
extern void page_frame_init(void) {

	uint32_t p = (uint32_t)_kernel_end;
	if (p >= 0xC0000000) p -= 0xC0000000;

	/* last frame of kernel */
	page_frame_id_t fr = p / 4096;

	for (uint32_t i = 0; i <= fr+128; i++) {

		if (i < LOWMEM_START || i >= LOWMEM_START+LOWMEM_SIZE)
			page_frame_use(i);
		//	bitmap[i / 8] |= 1 << (i % 8);
	}
}

/* allocate a frame */
extern page_frame_id_t page_frame_alloc(void) {

	for (uint32_t i = 0; i < BITMAP_SIZE; i++) {

		for (int j = 0; j < 8; j++) {

			if (!(bitmap[i] & (1 << j))) {

				page_frame_id_t p = i * 8 + j;
				page_frame_use(p);
				return p;
			}
		}
	}
	return 0;
}

/* set frame to used */
extern void page_frame_use(page_frame_id_t id) {

	uint32_t byte = id / 8;
	uint8_t bit = (uint8_t)(1 << (id % 8));

	if (!(bitmap[byte] & bit)) nused++;
	bitmap[byte] |= bit;
}

/* set frame to free */
extern void page_frame_free(page_frame_id_t id) {

	uint32_t byte = id / 8;
	uint8_t bit = (uint8_t)(1 << (id % 8));

	if (bitmap[byte] & bit) nused--;
	bitmap[byte] &= ~(bit);
}

/* get number of used frames */
extern uint32_t page_frame_get_used_count(void) {

	return nused;
}

/* initialize page mapper */
extern void page_init(void) {

	page_frame_init();

	/* higher-half bootstrap should've initialized paging */
	uint32_t cr3;
	__asm__("mov %%cr3, %0": "=r"(cr3));

	page_dir = (page_dir_entry_t *)(cr3 + 0xC0000000);
	page_dir_wrap = NULL;
	page_start = (uint32_t)_kernel_end / 4096 + 1;

	/* map the page dir entry to the page dir */
	page_id_t pt = page_dir_find_entry();
	page_map_table(pt, cr3 / 4096); /* divided by 4K for the frame id */
	page_table_id = pt << 10;
	page_table = (page_tab_entry_t *)(page_table_id << 12);
	page_start = page_table_id + 1024;

	page_breakp = page_start;
	page_dir_wrap = page_dir;
}

/* map all page tables in kernel area */
extern void page_init_top(void) {

	/* allocate remaining page tables (makes it much simpler to clone a directory but keep the kernel sections in tact) */
	page_id_t pt = ALIGN(page_breakp, 1024) / 1024;
	for (page_id_t i = pt; i < 1024; i++)
		page_map_table(i, page_frame_alloc());
}

/* find free page directory entry */
extern page_id_t page_dir_find_entry(void) {

	page_id_t pt = 0;
	for (pt = page_start / 1024; pt < 1024; pt++) {
		if (!page_dir[pt]) break;
	}
	if (pt >= 1024) return 0;
	return pt;
}

/* map page table */
extern void page_map_table(page_id_t p, page_frame_id_t f) {

	if (page_dir_wrap) page_dir_wrap[p] = PAGE_ENT(f) | PAGE_FLAG_P | PAGE_FLAG_RW;
	else page_dir[p] = PAGE_ENT(f) | PAGE_FLAG_P | PAGE_FLAG_RW;
	
	/* invalidate entry in tlb to allow for editing of the table */
	page_invalidate(page_table_id + p);

	for (page_id_t i = 0; i < 1024; i++) {
		if (page_table) {
			
			page_table[p*1024+i] = 0;
			page_invalidate(p*1024+i);
		}
	}
}

/* map page to frame */
extern void page_map(page_id_t p, page_frame_id_t f) {

	page_id_t pt = p/1024;

	/* map table if necessary */
	if (!page_dir_wrap[pt]) {
		
		page_frame_id_t fr = page_frame_alloc();
		page_map_table(pt, fr);
	}

	/* map page */
	page_table[p] = PAGE_ENT(f) | PAGE_FLAG_P | PAGE_FLAG_RW;
	page_invalidate(p);
}

/* invalidate an entry in the tlb */
extern void page_invalidate(page_id_t p) {

	uint32_t addr = p << 12;
	asm volatile("invlpg (%0)": : "r"(addr));
}

/* check if page is mapped */
extern bool page_is_mapped(page_id_t p) {

	if (page_dir_wrap[p/1024] && page_table[p])
		return true;
	return false;
}

/* get frame from page */
extern page_frame_id_t page_get_frame(page_id_t p) {

	if (!page_dir_wrap[p/1024]) return 0;
	return (page_table[p] & 0xfffff000) / 4096;
}

/* unmap page */
extern void page_unmap(page_id_t p) {

	if (!page_dir_wrap[p/1024]) return;

	page_table[p] = 0;
	page_invalidate(p);
}

/* get kernel page directory */
extern void *page_get_directory(void) {

	return (void *)page_dir - 0xC0000000;
}

/* clone kernel page directory */
extern void *page_clone_directory(page_frame_id_t frame, page_id_t page) {

	void *dir = (void *)(page * 4096);
	memcpy(dir, page_dir, 4096);

	/* map page directory as a page table */
	page_id_t pt = page_table_id >> 10;
	((uint32_t *)dir)[pt] = PAGE_ENT(frame) | PAGE_FLAG_P | PAGE_FLAG_RW;

	return (void *)(frame * 4096);
}
