#ifndef ECLAIR_DRIVER_FB_H
#define ECLAIR_DRIVER_FB_H

#include <e.clair/types.h>
#include <e.clair/multiboot.h>

extern void *fb_addr; /* direct pointer to framebuffer */
extern uint32_t fb_width; /* width of framebuffer */
extern uint32_t fb_height; /* height of framebuffer */
extern uint32_t fb_pitch;
extern uint8_t fb_bpp;

/* functions */
extern void fb_map(multiboot_saved_info_t *info); /* map framebuffer into memory */

#endif /* ECLAIR_DRIVER_FB_H */
