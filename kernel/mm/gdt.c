#include <e.clair/types.h>
#include <e.clair/string.h>
#include <e.clair/mm/gdt.h>

static gdt_segment_descriptor_t gdt[GDT_SEGMENT_COUNT];
static gdt_descriptor_t gdt_desc;

/* initialize gdt with two segments per privelige level */
extern void gdt_init(void) {

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

	/* descriptor */
	gdt_desc.size = sizeof(gdt) - 1;
	gdt_desc.addr = (unsigned long)gdt;

	/* load gdt */
	__asm__("lgdt (gdt_desc)\n"
		"mov $16,%ax\n"
		"mov %ax,%ds\n"
		"mov %ax,%es\n"
		"mov %ax,%fs\n"
		"mov %ax,%gs\n"
		"jmp $8,$random_label\n"
		"random_label:");
}
