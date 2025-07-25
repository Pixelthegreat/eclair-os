/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_IDT_H
#define ECLAIR_IDT_H

#include <kernel/types.h>

typedef struct idt_descriptor {
	uint16_t size; /* size of descriptor */
	uint32_t addr; /* address */
} __attribute__((packed)) idt_descriptor_t;

/* gate descriptor */
typedef struct idt_gate_descriptor {
	uint16_t off0; /* low offset bytes */
	uint16_t segsel; /* segment selector in gdt */
	uint8_t rsvd; /* reserved */
	uint8_t flags; /* flags */
	uint16_t off1; /* high offset bytes */
} __attribute__((packed)) idt_gate_descriptor_t;

#define IDT_GATE_TYPE_TASK 0x5
#define IDT_GATE_TYPE_INT16 0x6
#define IDT_GATE_TYPE_TRAP16 0x7
#define IDT_GATE_TYPE_INT 0xe
#define IDT_GATE_TYPE_TRAP 0xf

#define IDT_GATE_FLAG_DPLU 0b01100000
#define IDT_GATE_FLAG_P 0b10000000

/* register info */
typedef struct idt_regs {
	uint32_t ds; /* data segment */
	/* gp registers */
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	/* pushed by isr stub */
	uint32_t n_int, err_code;
	/* pushed by cpu */
	uint32_t eip, cs, eflags, useresp, ss;
} idt_regs_t;

typedef void (*idt_isr_t)(idt_regs_t *regs); /* custom handler */

#define IDT_ISR_GPFAULT 13
#define IDT_ISR_PGFAULT 14

#define IDT_INT_SYSCALL 0x80

/* functions */
extern void idt_init(void); /* initialize */
extern void idt_enable(void); /* enable interrupts */
extern void idt_set_gate(uint32_t n, void *p, uint8_t tp); /* set handler */
extern void idt_isr_handler(idt_regs_t *regs); /* main isr handler */
extern void idt_irq_handler(idt_regs_t *regs); /* main irq handler */
extern void idt_int_handler(idt_regs_t *regs); /* main int handler */
extern void idt_set_isr_callback(uint32_t n, idt_isr_t isr); /* set isr callback */
extern void idt_set_irq_callback(uint32_t n, idt_isr_t isr); /* set irq callback */
extern void idt_send_eoi(void); /* send eoi command to first pic */
extern void idt_disable_irq_eoi(uint32_t n); /* disable automatic eoi command for irqs */

/* PIC ports and commands */
#define IDT_PIC0_CMD 0x20
#define IDT_PIC0_DATA 0x21
#define IDT_PIC1_CMD 0xa0
#define IDT_PIC1_DATA 0xa1

#define IDT_PIC_CMD_ICW1 0x11
#define IDT_PIC_CMD_EOI 0x20

/* isrs (should not be called directly) */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

extern void sysint();

#endif /* ECLAIR_IDT_H */
