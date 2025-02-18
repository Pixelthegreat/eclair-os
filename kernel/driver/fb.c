#include <kernel/types.h>
#include <kernel/multiboot.h>
#include <kernel/mm/paging.h>
#include <kernel/driver/fb.h>

static uint32_t bytes; /* bytes per pixel */

void *fb_addr = NULL;
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_pitch = 0;
uint8_t fb_bpp = 0;
fb_format_t fb_format = {};

const fb_format_t FB_RGB = {
	.r = {0, 0xff},
	.g = {1, 0xff},
	.b = {2, 0xff},
};

/* map framebuffer into memory */
extern void fb_map(multiboot_saved_info_t *info, fb_format_t format) {

	if (!info->f_framebuf) return;

	/* allocate pages */
	page_frame_id_t fr = ((uint32_t)info->fb_addr) / 4096;
	uint32_t poff = ((uint32_t)info->fb_addr) % 4096;

	page_frame_use(fr);
	page_id_t page = page_breakp++;
	page_map(page, fr);

	bytes = (uint32_t)info->fb_bpp / 8;
	uint32_t frcnt = (poff + (info->fb_height * info->fb_pitch * bytes)) / 4096 + 1;
	for (uint32_t i = fr+1; i < fr+frcnt; i++) {

		page_frame_use(i);
		page_map(page_breakp++, i);
	}

	/* set address */
	fb_addr = PAGE_ADDR(page) + poff;
	fb_width = info->fb_width;
	fb_height = info->fb_height;
	fb_pitch = info->fb_pitch;
	fb_bpp = info->fb_bpp;
	fb_format = format;
}

/* set pixel to color */
extern void fb_set_pixel(uint32_t x, uint32_t y, fb_color_t color) {

	if (x >= fb_width || y >= fb_height) return;

	uint8_t *addr = (uint8_t *)(fb_addr + y * fb_pitch + x * bytes);

	addr[fb_format.r.index] = color.r;
	addr[fb_format.g.index] = color.g;
	addr[fb_format.b.index] = color.b;
}
