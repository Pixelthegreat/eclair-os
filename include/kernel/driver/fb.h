#ifndef ECLAIR_DRIVER_FB_H
#define ECLAIR_DRIVER_FB_H

#include <kernel/types.h>
#include <kernel/boot.h>

/* color format */
typedef struct fb_format {
	struct {
		uint32_t masksz; /* field mask size */
		uint32_t pos; /* field position */
	} r, g, b;
	uint32_t bytes; /* bytes per pixel */
} fb_format_t;

typedef struct fb_color {
	uint8_t r, g, b;
} fb_color_t;

/* font info */
typedef struct fb_font {
	uint32_t w, h; /* width and height of characters */
	uint32_t hspace; /* horizontal spacing */
	uint8_t *data; /* font data */
	bool flip; /* horizontally flip characters */
} fb_font_t;

/* compare color formats */
static inline bool fb_format_is(fb_format_t *a, fb_format_t *b) {

	if (a == b) return true;

	return (a->r.masksz == b->r.masksz && a->r.pos == b->r.pos) &&
	       (a->g.masksz == b->g.masksz && a->g.pos == b->g.pos) &&
	       (a->b.masksz == b->b.masksz && a->b.pos == b->b.pos) &&
	       a->bytes == b->bytes;
}

/* framebuffer attributes */
extern void *fb_addr; /* direct pointer to framebuffer */
extern uint32_t fb_width; /* width of framebuffer */
extern uint32_t fb_height; /* height of framebuffer */
extern uint32_t fb_pitch; /* number of bytes in a line */
extern uint8_t fb_bpp; /* bits per pixel */
extern fb_format_t fb_format; /* framebuffer format */
extern fb_font_t fb_font; /* framebuffer text font */

extern fb_format_t FB_RGB; /* rgba format */
extern fb_format_t FB_GRAY; /* grayscale format */

/* functions */
extern void fb_map(boot_saved_info_t *info, fb_format_t format); /* map framebuffer into memory */
extern void fb_set_pixel(uint32_t x, uint32_t y, fb_color_t color); /* set pixel to color */
extern void fb_copy_area(uint32_t dstx, uint32_t dsty, uint32_t w, uint32_t h, void *data, fb_format_t *format); /* copy area to framebuffer memory */
extern void fb_text(uint32_t x, uint32_t y, const char *text, fb_color_t color, fb_color_t bgcolor); /* draw text */
extern void fb_scroll(uint32_t y); /* scroll framebuffer up */

#endif /* ECLAIR_DRIVER_FB_H */
