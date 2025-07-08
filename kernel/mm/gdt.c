/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/mm/gdt.h>

static gdt_segment_descriptor_t gdt[GDT_SEGMENT_COUNT];
static gdt_descriptor_t gdt_desc;

gdt_tss_t gdt_tss;

extern void kernel_stack_top(void);

/* initialize gdt with two segments per privelige level */
extern void gdt_init(void) {

	memset(&gdt_tss, 0, sizeof(gdt_tss));
	gdt_tss.esp0 = (uint32_t)kernel_stack_top;
	gdt_tss.ss0 = 16;

	/* supervisor */
	gdt[GDT_SEGMENT_SP_CODE].lim0 = 0xFFFF;
	gdt[GDT_SEGMENT_SP_CODE].acc = GDT_SEGMENT_ACCESS_RW |
				       GDT_SEGMENT_ACCESS_E |
				       GDT_SEGMENT_ACCESS_S |
				       GDT_SEGMENT_ACCESS_P;
	gdt[GDT_SEGMENT_SP_CODE].lim1 = 0xF;
	gdt[GDT_SEGMENT_SP_CODE].flags = GDT_SEGMENT_FLAG_DB |
					 GDT_SEGMENT_FLAG_G;
	
	gdt[GDT_SEGMENT_SP_DATA].lim0 = 0xFFFF;
	gdt[GDT_SEGMENT_SP_DATA].acc = GDT_SEGMENT_ACCESS_RW |
				       GDT_SEGMENT_ACCESS_S |
				       GDT_SEGMENT_ACCESS_P;
	gdt[GDT_SEGMENT_SP_DATA].lim1 = 0xF;
	gdt[GDT_SEGMENT_SP_DATA].flags = GDT_SEGMENT_FLAG_DB |
					 GDT_SEGMENT_FLAG_G;

	/* user */
	gdt[GDT_SEGMENT_US_CODE].lim0 = 0xFFFF;
	gdt[GDT_SEGMENT_US_CODE].acc = GDT_SEGMENT_ACCESS_RW |
				       GDT_SEGMENT_ACCESS_E |
				       GDT_SEGMENT_ACCESS_S |
				       GDT_SEGMENT_ACCESS_DPLU |
				       GDT_SEGMENT_ACCESS_P;
	gdt[GDT_SEGMENT_US_CODE].lim1 = 0xF;
	gdt[GDT_SEGMENT_US_CODE].flags = GDT_SEGMENT_FLAG_DB |
					 GDT_SEGMENT_FLAG_G;

	gdt[GDT_SEGMENT_US_DATA].lim0 = 0xFFFF;
	gdt[GDT_SEGMENT_US_DATA].acc = GDT_SEGMENT_ACCESS_RW |
				       GDT_SEGMENT_ACCESS_S |
				       GDT_SEGMENT_ACCESS_DPLU |
				       GDT_SEGMENT_ACCESS_P;
	gdt[GDT_SEGMENT_US_DATA].lim1 = 0xF;
	gdt[GDT_SEGMENT_US_DATA].flags = GDT_SEGMENT_FLAG_DB |
					 GDT_SEGMENT_FLAG_G;

	uint32_t tss_base = (uint32_t)&gdt_tss;

	gdt[GDT_SEGMENT_TSS].lim0 = sizeof(gdt_tss) & 0xffff;
	gdt[GDT_SEGMENT_TSS].acc = GDT_SEGMENT_ACCESS_A |
				   GDT_SEGMENT_ACCESS_E |
				   GDT_SEGMENT_ACCESS_P;
	gdt[GDT_SEGMENT_TSS].lim1 = (sizeof(gdt_tss) >> 16) & 0xf;
	gdt[GDT_SEGMENT_TSS].flags = 0;
	gdt[GDT_SEGMENT_TSS].base0 = tss_base & 0xffffff;
	gdt[GDT_SEGMENT_TSS].base2 = (tss_base >> 24) & 0xff;

	/* descriptor */
	gdt_desc.size = sizeof(gdt) - 1;
	gdt_desc.addr = (unsigned long)gdt;

	/* load gdt */
	asm volatile("lgdt (gdt_desc)\n"
			 "mov $16,%ax\n"
			 "mov %ax,%ds\n"
			 "mov %ax,%es\n"
			 "mov %ax,%fs\n"
			 "mov %ax,%gs\n"
			 "jmp $8,$random_label\n"
			 "random_label:");
	gdt_flush_tss();
}

/* flush tss */
extern void gdt_flush_tss(void) {

	asm volatile("mov $40,%ax\nltr %ax\n");
}
