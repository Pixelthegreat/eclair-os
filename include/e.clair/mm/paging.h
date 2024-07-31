#ifndef ECLAIR_MM_PAGING_H
#define ECLAIR_MM_PAGING_H

#include <e.clair/types.h>

typedef uint32_t page_dir_entry_t; /* page directory entry */
typedef uint32_t page_tab_entry_t; /* page table entry */
typedef uint32_t page_frame_id_t; /* frame id */
typedef uint32_t page_id_t; /* page id */

#define PAGE_ENT(a) ((page_dir_entry_t)(a) << 12)
#define PAGE_ADDR(id) ((void *)((id) << 12))

/* entry flags */
#define PAGE_FLAG_P 0x1
#define PAGE_FLAG_RW 0x2
#define PAGE_FLAG_US 0x4
#define PAGE_FLAG_PWT 0x8
#define PAGE_FLAG_PCD 0x10
#define PAGE_FLAG_A 0x20
#define PAGE_FLAG_D 0x40
#define PAGE_FLAG_PS 0x80
#define PAGE_FLAG_G 0x100

/* functions */
extern void page_frame_init(void); /* initialize frame allocator */
extern page_frame_id_t page_frame_alloc(void); /* allocate a frame */
extern void page_frame_use(page_frame_id_t id); /* set frame to used */
extern void page_frame_free(page_frame_id_t id); /* set frame to free */

extern void page_init(void); /* initialize page mapper */
extern page_id_t page_dir_find_entry(void); /* find free page directory entry */
extern void page_map_table(page_id_t p, page_frame_id_t f); /* map a page table */
extern void page_map(page_id_t p, page_frame_id_t f); /* map a page to a frame */
extern page_id_t page_alloc(uint32_t n, page_frame_id_t *flist); /* allocate a contiguous number of pages */
extern void page_invalidate(page_id_t p); /* invalidate an entry in the tlb */
extern bool page_is_mapped(page_id_t p); /* check if page is mapped */
extern page_frame_id_t page_get_frame(page_id_t p); /* get frame from page */
extern void page_unmap(page_id_t p); /* unmap page */
extern void *page_get_directory(void); /* get kernel page directory */

#endif /* ECLAIR_MM_PAGING_H */
