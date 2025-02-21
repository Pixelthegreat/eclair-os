#ifndef ECLAIR_MULTIBOOT_H
#define ECLAIR_MULTIBOOT_H

#include <kernel/types.h>

typedef struct multiboot_tag {
	uint32_t type;
	uint32_t size;
} __attribute__((packed)) multiboot_tag_t;

/* memory tag */
#define MULTIBOOT_TAG_MEM 4

typedef struct multiboot_mem_tag {
	multiboot_tag_t tag;
	uint32_t memlow;
	uint32_t memup;
} __attribute__((packed)) multiboot_mem_tag_t;

/* bios boot device */
#define MULTIBOOT_TAG_DEV 5

typedef struct multiboot_dev_tag {
	multiboot_tag_t tag;
	uint32_t biosdev;
	uint32_t part;
	uint32_t subpart;
} __attribute__((packed)) multiboot_dev_tag_t;

/* command line */
#define MULTIBOOT_TAG_CMDLINE 1

typedef struct multiboot_cmdline_tag {
	multiboot_tag_t tag;
	char cmdline[0];
} __attribute__((packed)) multiboot_cmdline_tag_t;

/* framebuffer info */
#define MULTIBOOT_TAG_FRAMEBUFFER 8

#define MULTIBOOT_FB_TYPE_RGB 1

typedef struct multiboot_framebuffer_color_info {
	struct {
		uint8_t fpos; /* field position */
		uint8_t masksz; /* mask size */
	} red, green, blue;
} __attribute__((packed)) multiboot_framebuffer_color_info_t;

typedef struct multiboot_framebuffer_tag {
	multiboot_tag_t tag;
	uint32_t loaddr; /* low 32 bits of address */
	uint32_t hiaddr; /* high 32 bits of address */
	uint32_t pitch; /* number of bytes between rows */
	uint32_t width; /* width of framebuffer */
	uint32_t height; /* height of framebuffer */
	uint8_t bpp; /* bits per pixel */
	uint8_t type; /* type of framebuffer */
	uint8_t reserved[2]; /* reserved data */
	multiboot_framebuffer_color_info_t color; /* color info */
} __attribute__((packed)) multiboot_framebuffer_tag_t;

/* memory map */
#define MULTIBOOT_TAG_MEMMAP 6
#define MULTIBOOT_MEMMAP_AVAIL 1

#define MULTIBOOT_ADDR32(a) ((uint32_t)((a) & 0xffffffff))

typedef struct multiboot_memmap_entry {
	uint64_t addr; /* base address */
	uint64_t length; /* length in bytes */
	uint32_t type; /* type of memory region */
	uint32_t reserved; /* reserved data */
} __attribute__((packed)) multiboot_memmap_entry_t;

typedef struct multiboot_memmap_tag {
	multiboot_tag_t tag;
	uint32_t entsize; /* entry size */
	uint32_t entversion; /* entry version */
	multiboot_memmap_entry_t entries[]; /* entries */
} __attribute__((packed)) multiboot_memmap_tag_t;

/* main info */
typedef struct multiboot_info {
	uint32_t size;
	uint32_t rsvd;
	multiboot_tag_t tags[0];
} __attribute__((packed)) multiboot_info_t;

/* useful structure for saved info */
typedef struct multiboot_saved_info {
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
	multiboot_framebuffer_color_info_t *fb_color; /* color info */
	uint32_t mm_nentries; /* number of memory map entries */
	uint32_t mm_entsize; /* entry size */
} multiboot_saved_info_t;

/* command line info */
typedef struct multiboot_cmdline {
	bool uart_tty; /* initialize uart as tty device */
	bool quiet; /* do not display log messages under info or warning */
} multiboot_cmdline_t;

/* preloaded values */
extern multiboot_info_t *multiboot_data_info;
extern uint32_t multiboot_data_magic;

/* functions */
extern void multiboot_init(void); /* initialize */
extern void multiboot_map_structure(void); /* map the structure */
extern multiboot_info_t *multiboot_get_structure(void); /* get pointer to structure */
extern multiboot_saved_info_t *multiboot_get_saved_info(void); /* get info that was saved from structure */
extern multiboot_cmdline_t *multiboot_get_cmdline(void); /* get command line info */

#endif /* ECLAIR_MULTIBOOT_H */
