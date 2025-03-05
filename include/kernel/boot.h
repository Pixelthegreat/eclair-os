#ifndef ECLAIR_BOOT_H
#define ECLAIR_BOOT_H

#include <kernel/types.h>

/* supported boot protocols */
typedef enum boot_protocol {
	BOOT_PROTOCOL_S3BOOT = 0,
	BOOT_PROTOCOL_MULTIBOOT,

	BOOT_PROTOCOL_COUNT,
} boot_protocol_t;

/* useful structure for saved info */
typedef struct boot_saved_info {
	bool f_cmdline; /* found command line tag */
	bool f_memlayout; /* found memory layout tag */
	bool f_bootdev; /* found boot device */
	bool f_framebuf; /* found framebuffer */
	bool f_memmap; /* found memory map */
	const char *cmdline; /* boot command line */
	uint32_t memlow; /* lower memory limit */
	uint32_t memup; /* upper memory limit */
	uint32_t biosdev; /* bios boot device */
	uint32_t part; /* partition */
	uint32_t subpart; /* sub-partition */
	void *fb_addr; /* framebuffer address */
	uint32_t fb_pitch; /* space in bytes between lines */
	uint32_t fb_width; /* framebuffer width */
	uint32_t fb_height; /* framebuffer height */
	uint32_t fb_bpp; /* bits per pixel */
	uint32_t mm_nentries; /* number of memory map entries */
	uint32_t mm_entsize; /* entry size */
} boot_saved_info_t;

/* boot structure */
typedef enum boot_struct_type {
	BOOT_STRUCT_CMDLINE = 0,
	BOOT_STRUCT_MEMMAP,
	BOOT_STRUCT_FRAMEBUF,

	BOOT_STRUCT_COUNT,
} boot_struct_type_t;

typedef struct boot_info {
	uint32_t checksum; /* checksum of structure */
	uint32_t size; /* size of structure */
	uint32_t offsets[BOOT_STRUCT_COUNT]; /* offsets of substructures */
} __attribute__((packed)) boot_info_t;

/* boot framebuffer */
#define BOOT_FRAMEBUF_TYPE_RGB 1

typedef struct boot_framebuf_info {
	uint32_t type; /* type of framebuffer data */
	uint32_t addr; /* address of framebuffer */
	uint32_t pitch; /* bytes per scanline */
	uint32_t width; /* width of image */
	uint32_t height; /* height of image */
	uint32_t depth; /* bits per pixel */
	uint8_t rmask_sz; /* size of red mask */
	uint8_t rmask_pos; /* position of red mask */
	uint8_t gmask_sz; /* size of green mask */
	uint8_t gmask_pos; /* position of green mask */
	uint8_t bmask_sz; /* size of blue mask */
	uint8_t bmask_pos; /* position of blue mask */
} __attribute__((packed)) boot_framebuf_t;

/* command line info */
typedef struct boot_cmdline {
	bool uart_tty; /* initialize uart as tty device */
	bool quiet; /* do not display log messages under info or warning */
} boot_cmdline_t;

extern boot_protocol_t boot_protocol;

extern boot_info_t *boot_data_info;

/* functions */
extern void boot_init(void); /* initialize */
extern boot_info_t *boot_get_info(void); /* get page mapped info */
extern boot_saved_info_t *boot_get_saved_info(void); /* get useful saved info */
extern boot_cmdline_t *boot_get_cmdline(void); /* get command line info */

#endif /* ECLAIR_BOOT_H */
