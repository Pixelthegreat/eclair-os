#ifndef ECLAIR_DRIVER_FB_H
#define ECLAIR_DRIVER_FB_H

#include <kernel/types.h>
#include <kernel/multiboot.h>

/* color format */
typedef struct fb_format {
	struct {
		uint32_t index; /* field index */
		uint32_t mask; /* field mask */
	} r, g, b;
} fb_format_t;

typedef struct fb_color {
	uint8_t r, g, b;
} fb_color_t;

extern void *fb_addr; /* direct pointer to framebuffer */
extern uint32_t fb_width; /* width of framebuffer */
extern uint32_t fb_height; /* height of framebuffer */
extern uint32_t fb_pitch; /* number of bytes in a line */
extern uint8_t fb_bpp; /* bits per pixel */
extern fb_format_t fb_format; /* framebuffer format */

extern const fb_format_t FB_RGB; /* rgba format */

/* functions */
extern void fb_map(multiboot_saved_info_t *info, fb_format_t format); /* map framebuffer into memory */
extern void fb_set_pixel(uint32_t x, uint32_t y, fb_color_t color); /* set pixel to color */

#endif /* ECLAIR_DRIVER_FB_H */
