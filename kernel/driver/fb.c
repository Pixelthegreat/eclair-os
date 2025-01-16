#include <kernel/types.h>
#include <kernel/multiboot.h>
#include <kernel/mm/paging.h>
#include <kernel/driver/fb.h>

void *fb_addr = NULL;
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_pitch = 0;
uint8_t fb_bpp = 0;

/* map framebuffer into memory */
extern void fb_map(multiboot_saved_info_t *info) {

	if (!info->f_framebuf) return;

	/* allocate pages */
	page_frame_id_t fr = ((uint32_t)info->fb_addr) / 4096;
	uint32_t poff = ((uint32_t)info->fb_addr) % 4096;

	page_frame_use(fr);
	page_id_t page = page_alloc(1, &fr);

	uint32_t frcnt = (poff + (info->fb_height * info->fb_pitch * 3)) / 4096 + 1;
	for (uint32_t i = fr+1; i < fr+frcnt; i++) {

		page_frame_use(i);
		page_map(page+i, i);
	}

	/* set address */
	fb_addr = PAGE_ADDR(page) + poff;
	fb_width = info->fb_width;
	fb_height = info->fb_height;
	fb_pitch = info->fb_pitch;
	fb_bpp = info->fb_bpp;
}
