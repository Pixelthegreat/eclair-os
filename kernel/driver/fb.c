#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/boot.h>
#include <kernel/mm/paging.h>
#include <kernel/driver/fbfont.h>
#include <kernel/driver/fb.h>

void *fb_addr = NULL;
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_pitch = 0;
uint8_t fb_bpp = 0;
fb_format_t fb_format = {};
fb_font_t fb_font = {8, 16, 1, fb_vga_font, true};

/* format constants */
fb_format_t FB_RGB = {
	.r = {8, 0},
	.g = {8, 8},
	.b = {8, 16},
	.bytes = 3,
};
fb_format_t FB_GRAY = {
	.r = {8, 0},
	.g = {8, 0},
	.b = {8, 0},
	.bytes = 1,
};

/* map framebuffer into memory */
extern void fb_map(boot_saved_info_t *info, fb_format_t format) {

	if (!info->f_framebuf) return;

	/* allocate pages */
	page_frame_id_t fr = (uint32_t)info->fb_addr / 4096;
	uint32_t poff = (uint32_t)info->fb_addr % 4096;

	page_id_t page = page_breakp;
	uint32_t frcnt = ALIGN(poff + (info->fb_height * info->fb_pitch), 4096) / 4096;
	for (uint32_t i = fr; i < fr+frcnt; i++) {

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

	/* clear screen */
	fb_color_t black = {0, 0, 0};
	for (uint32_t y = 0; y < fb_height; y++) {
		for (uint32_t x = 0; x < fb_width; x++) {
			fb_set_pixel(x, y, black);
		}
	}
}

/* set pixel to color */
extern void fb_set_pixel(uint32_t x, uint32_t y, fb_color_t color) {

	if (x >= fb_width || y >= fb_height) return;

	uint8_t *addr = (uint8_t *)(fb_addr + y * fb_pitch + x * fb_format.bytes);

	uint32_t pixel = 0;

	/* converts an r8g8b8 format color to a pixel value */
	pixel |= ((uint32_t)color.r * fb_format.r.masksz / 8) << fb_format.r.pos;
	pixel |= ((uint32_t)color.g * fb_format.g.masksz / 8) << fb_format.g.pos;
	pixel |= ((uint32_t)color.b * fb_format.b.masksz / 8) << fb_format.b.pos;
	
	for (uint32_t i = 0; i < fb_format.bytes; i++)
		addr[i] = (uint8_t)((pixel >> (i * 8)) & 0xff);
}

/* get pixel color */
extern fb_color_t fb_get_pixel(uint32_t x, uint32_t y) {

	if (x >= fb_width || y >= fb_height) return (fb_color_t){0, 0, 0};

	uint8_t *addr = (uint8_t *)(fb_addr + y * fb_pitch + x * fb_format.bytes);

	uint32_t pixel = 0;
	for (uint32_t i = 0; i < fb_format.bytes; i++)
		pixel |= (uint32_t)addr[i] << (i * 8);

	/* convert a pixel to r8g8b8 format */
	fb_color_t color;
	color.r = (uint8_t)(((pixel >> fb_format.r.pos) & (0xffffffff >> (32-fb_format.r.pos))) * 8 / fb_format.r.masksz);
	color.g = (uint8_t)(((pixel >> fb_format.g.pos) & (0xffffffff >> (32-fb_format.g.pos))) * 8 / fb_format.g.masksz);
	color.b = (uint8_t)(((pixel >> fb_format.b.pos) & (0xffffffff >> (32-fb_format.b.pos))) * 8 / fb_format.b.masksz);

	return color;
}

/* copy area to framebuffer memory */
extern void fb_copy_area(uint32_t dstx, uint32_t dsty, uint32_t w, uint32_t h, void *data, fb_format_t *format) {

	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {

			uint8_t *addr = (uint8_t *)(data + (y * w + x) * format->bytes);

			fb_color_t color;
			
			uint32_t pixel = 0;
			for (uint32_t i = 0; i < format->bytes; i++)
				pixel |= (uint32_t)addr[i] << (i * 8);

			/* converts a pixel value to r8g8b8 format */
			color.r = (uint8_t)(((pixel >> format->r.pos) & (0xffffffff >> (32-format->r.pos))) * 8 / format->r.masksz);
			color.g = (uint8_t)(((pixel >> format->g.pos) & (0xffffffff >> (32-format->g.pos))) * 8 / format->g.masksz);
			color.b = (uint8_t)(((pixel >> format->b.pos) & (0xffffffff >> (32-format->b.pos))) * 8 / format->b.masksz);

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
			for (uint32_t px = 0; px < fb_font.w+fb_font.hspace; px++) {

				uint32_t bit = fb_font.flip? fb_font.w-1-px: px;

				uint32_t dx = x + pos * (fb_font.w + fb_font.hspace) + px;
				uint32_t dy = y + py;

				if (px < fb_font.w && bitmap[py+bit/fb_font.w] & (uint8_t)(1 << (bit%8)))
					fb_set_pixel(dx, dy, color);
				else fb_set_pixel(dx, dy, bgcolor);
			}
		}
		pos++;
	}
}

/* theoretically faster memcpy and memset */
static void *memcpy32(void *dst, void *src, size_t cnt) {

	while (cnt) {

		if (cnt > 4) {

			*(uint32_t *)dst = *(uint32_t *)src;
			dst += 4;
			src += 4;
			cnt -= 4;
		}
		else {

			*(uint8_t *)dst++ = *(uint8_t *)src++;
			cnt--;
		}
	}
	return src;
}

static void *memset32(void *dst, int ch, size_t cnt) {

	uint32_t val = (uint32_t)ch * 0x1010101;
	while (cnt) {

		if (cnt > 4) {

			*(uint32_t *)dst = val;
			dst += 4;
			cnt -= 4;
		}
		else {

			*(uint8_t *)dst++ = (uint8_t)ch;
			cnt--;
		}
	}
	return dst;
}

/* scroll framebuffer up */
extern void fb_scroll(uint32_t y) {

	uint32_t sz = fb_height*fb_pitch;
	uint32_t dist = y*fb_pitch;
	uint32_t top = sz-dist;

	for (uint32_t py = y; py < fb_height; py++) {

		void *src = fb_addr+py*fb_pitch;
		void *dst = fb_addr+(py-y)*fb_pitch;

		memcpy32(dst, src, fb_width*fb_format.bytes);
	}
	for (uint32_t py = 0; py < y; py++)
		memset32(fb_addr+(fb_height-y+py)*fb_pitch, 0, fb_width*fb_format.bytes);
}
