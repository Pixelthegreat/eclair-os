#include <e.clair/types.h>
#include <e.clair/tty.h>
#include <e.clair/mm/paging.h>
#include <e.clair/multiboot.h>

#define NBUFSZ 12
static page_frame_id_t frames[NBUFSZ]; /* buffer to store page frame numbers */

static multiboot_info_t *info = NULL;
static multiboot_saved_info_t saved;

/* initialize */
extern void multiboot_init(void) {

	multiboot_map_structure();

	/* read info */
	multiboot_tag_t *tagptr = info->tags;
	size_t tagsz = 0;
	while (tagptr->type != 0) {

		/* command line */
		if (tagptr->type == MULTIBOOT_TAG_CMDLINE) {

			multiboot_cmdline_tag_t *tag = (multiboot_cmdline_tag_t *)tagptr;
			saved.cmdline = tag->cmdline;
		}

		/* boot device */
		else if (tagptr->type == MULTIBOOT_TAG_DEV) {

			multiboot_dev_tag_t *tag = (multiboot_dev_tag_t *)tagptr;
			saved.biosdev = tag->biosdev;
			saved.part = tag->part;
			saved.subpart = tag->subpart;
		}

		/* memory */
		else if (tagptr->type == MULTIBOOT_TAG_MEM) {

			multiboot_mem_tag_t *tag = (multiboot_mem_tag_t *)tagptr;
			saved.memlow = tag->memlow;
			saved.memup = tag->memup;
		}

		/* increment */
		tagsz += ALIGN(tagptr->size, 8);
		tagptr = (void *)info->tags + tagsz;
	}
}

/* get multiboot structure */
extern void multiboot_map_structure(void) {

	page_frame_id_t fr = (uint32_t)multiboot_data_info / 4096;
	uint32_t poff = (uint32_t)multiboot_data_info - (fr * 4096);
	
	/* determine frames to allocate */
	size_t nfrs = 1; page_id_t page;
	page_frame_id_t frs[2] = {fr, fr+1};
	page_frame_use(fr);

	/* structure spans multiple pages */
	if (poff + sizeof(multiboot_info_t) > 4096) {

		page_frame_use(fr+1);
		nfrs++;
	}
	page = page_alloc(nfrs, frs);
	info = PAGE_ADDR(page) + poff;

	/* allocate remaining frames */
	uint32_t frcnt = (poff + info->size) / 4096 + 1;
	frcnt = frcnt > NBUFSZ? NBUFSZ: frcnt;

	for (int i = 0; i < frcnt - nfrs; i++) {

		frames[i] = fr + nfrs + i;
		page_frame_use(frames[i]);
	}
	(void)page_alloc(frcnt - nfrs, frames);
}

/* get structure */
extern multiboot_info_t *multiboot_get_structure(void) {

	return info;
}

/* get saved info */
extern multiboot_saved_info_t *multiboot_get_saved_info(void) {

	return &saved;
}
