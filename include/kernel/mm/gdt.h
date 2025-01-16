#ifndef ECLAIR_MM_GDT_H
#define ECLAIR_MM_GDT_H

#include <kernel/types.h>

/* main descriptor */
typedef struct gdt_descriptor {
	uint16_t size;
#ifdef ECLAIR64 /* to be used */
	uint64_t addr;
#else
	uint32_t addr;
#endif
} __attribute__((packed)) gdt_descriptor_t;

/* segment descriptor */
typedef struct gdt_segment_descriptor {
	uint32_t lim0:16; /* limit low */
	uint32_t base0:24; /* base low */
	uint32_t acc:8; /* access flags */
	uint32_t lim1:4; /* remainder of limit */
	uint32_t flags:4; /* other flags */
	uint32_t base2:8; /* remainder of base */
} __attribute__((packed)) gdt_segment_descriptor_t;

/* entries */
enum {
	GDT_SEGMENT_NULL_DESC = 0,
	GDT_SEGMENT_SP_CODE,
	GDT_SEGMENT_SP_DATA,
	GDT_SEGMENT_US_CODE,
	GDT_SEGMENT_US_DATA,

	GDT_SEGMENT_COUNT,
};

/* access flags */
#define GDT_SEGMENT_ACCESS_A 0x1 /* accessed */
#define GDT_SEGMENT_ACCESS_RW 0x2 /* read-write */
#define GDT_SEGMENT_ACCESS_DC 0x4 /* direction */
#define GDT_SEGMENT_ACCESS_E 0x8 /* executable */
#define GDT_SEGMENT_ACCESS_S 0x10 /* code or data */
#define GDT_SEGMENT_ACCESS_DPLU 0x60 /* 3rd ring/user mode */
#define GDT_SEGMENT_ACCESS_P 0x80 /* present */

/* normal flags */
#define GDT_SEGMENT_FLAG_L 0x2 /* long mode */
#define GDT_SEGMENT_FLAG_DB 0x4 /* size */
#define GDT_SEGMENT_FLAG_G 0x8 /* granularity */

/* functions */
extern void gdt_init(void);

#endif /* ECLAIR_MM_GDT_H */
