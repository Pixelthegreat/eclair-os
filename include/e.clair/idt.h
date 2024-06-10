#ifndef ECLAIR_IDT_H
#define ECLAIR_IDT_H

#include <stdint.h>

typedef struct idt_descriptor {
	uint16_t size; /* size of descriptor */
#ifdef ECLAIR64
	uint64_t addr;
#else
	uint32_t addr;
#endif
} idt_descriptor_t;

#endif /* ECLAIR_IDT_H */
