#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/multiboot.h>
#include <kernel/mm/paging.h>
#include <kernel/driver/fbfont.h>
#include <kernel/driver/fb.h>

static uint32_t bytes; /* bytes per pixel */

void *fb_addr = NULL;
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_pitch = 0;
uint8_t fb_bpp = 0;
fb_format_t fb_format = {};
fb_font_t fb_font = {8, 16, 1, fb_vga_font, true};

/* format constants */
fb_format_t FB_RGB = {
	.r = {0, 0xff},
	.g = {1, 0xff},
	.b = {2, 0xff},
	.bytes = 3,
};
fb_format_t FB_GRAY = {
	.r = {0, 0xff},
	.g = {0, 0xff},
	.b = {0, 0xff},
	.bytes = 1,
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

/* copy area to framebuffer memory */
extern void fb_copy_area(uint32_t dstx, uint32_t dsty, uint32_t w, uint32_t h, void *data, fb_format_t *format) {

	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {

			uint8_t *addr = (uint8_t *)(data + (y * w + x) * format->bytes);

			fb_color_t color;
			color.r = addr[format->r.index];
			color.g = addr[format->g.index];
			color.b = addr[format->b.index];

			fb_set_pixel(dstx+x, dsty+y, color);
		}
	}
}

/* draw text */
extern void fb_text(uint32_t x, uint32_t y, const char *text, fb_color_t color, fb_color_t bgcolor) {

	uint32_t pos = 0;
	char cc;
	while ((cc = *text++) != 0) {

		uint8_t *bitmap = &fb_font.data[(int)cc * fb_font.h];

		/* draw character */
		for (uint32_t py = 0; py < fb_font.h; py++) {
			for (uint32_t px = 0; px < fb_font.w; px++) {

				uint32_t bit = fb_font.flip? fb_font.w-1-px: px;

				uint32_t dx = x + pos * (fb_font.w + fb_font.hspace) + px;
				uint32_t dy = y + py;

				if (bitmap[py+bit/fb_font.w] & (uint8_t)(1 << (bit%8)))
					fb_set_pixel(dx, dy, color);
				else fb_set_pixel(dx, dy, bgcolor);
			}
		}
		pos++;
	}
}

/* scroll framebuffer up */
extern void fb_scroll(uint32_t y) {

	uint32_t start = y * fb_pitch;
	uint32_t size = fb_height * fb_pitch;
	for (uint32_t i = start; i < size; i++)
		((uint8_t *)fb_addr)[i-start] = ((uint8_t *)fb_addr)[i];

	memset(fb_addr+size-start, 0, (uint32_t)start);
}
