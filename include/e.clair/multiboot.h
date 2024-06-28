#ifndef ECLAIR_MULTIBOOT_H
#define ECLAIR_MULTIBOOT_H

#include <e.clair/types.h>

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

/* main info */
typedef struct multiboot_info {
	uint32_t size;
	uint32_t rsvd;
	multiboot_tag_t tags[0];
} __attribute__((packed)) multiboot_info_t;

/* useful structure for saved info */
typedef struct multiboot_saved_info {
	const char *cmdline; /* boot command line */
	uint32_t memlow; /* lower memory limit */
	uint32_t memup; /* upper memory limit */
	uint32_t biosdev; /* bios boot device */
	uint32_t part; /* partition */
	uint32_t subpart; /* sub-partition */
} multiboot_saved_info_t;

/* preloaded values */
extern multiboot_info_t *multiboot_data_info;
extern uint32_t multiboot_data_magic;

/* functions */
extern void multiboot_init(void); /* initialize */
extern void multiboot_map_structure(void); /* map the structure */
extern multiboot_info_t *multiboot_get_structure(void); /* get pointer to structure */
extern multiboot_saved_info_t *multiboot_get_saved_info(void); /* get info that was saved from structure */

#endif /* ECLAIR_MULTIBOOT_H */
