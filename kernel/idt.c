#include <kernel/types.h>
#include <kernel/io/port.h>
#include <kernel/mm/gdt.h>
#include <kernel/tty.h>
#include <kernel/panic.h>
#include <kernel/idt.h>

static idt_descriptor_t idt_desc;
static idt_gate_descriptor_t idt[256];
static idt_isr_t isrs[256];
static bool noeoi[8];

static inline void _wait(void) {

	port_outb(0x80, 0);
}

/* initialize */
extern void idt_init(void) {

	__asm__("cli");

	/* install isrs */
	idt_set_gate(0, isr0, IDT_GATE_TYPE_TRAP);
	idt_set_gate(1, isr1, IDT_GATE_TYPE_TRAP);
	idt_set_gate(2, isr2, IDT_GATE_TYPE_INT); /* nmi */
	idt_set_gate(3, isr3, IDT_GATE_TYPE_TRAP);
	idt_set_gate(4, isr4, IDT_GATE_TYPE_TRAP);
	idt_set_gate(5, isr5, IDT_GATE_TYPE_TRAP);
	idt_set_gate(6, isr6, IDT_GATE_TYPE_TRAP);
	idt_set_gate(7, isr7, IDT_GATE_TYPE_TRAP);
	idt_set_gate(8, isr8, IDT_GATE_TYPE_TRAP);
	idt_set_gate(9, isr9, IDT_GATE_TYPE_TRAP);
	idt_set_gate(10, isr10, IDT_GATE_TYPE_TRAP);
	idt_set_gate(11, isr11, IDT_GATE_TYPE_TRAP);
	idt_set_gate(12, isr12, IDT_GATE_TYPE_TRAP);
	idt_set_gate(13, isr13, IDT_GATE_TYPE_TRAP);
	idt_set_gate(14, isr14, IDT_GATE_TYPE_TRAP);
	idt_set_gate(15, isr15, IDT_GATE_TYPE_TRAP);
	idt_set_gate(16, isr16, IDT_GATE_TYPE_TRAP);
	idt_set_gate(17, isr17, IDT_GATE_TYPE_TRAP);
	idt_set_gate(18, isr18, IDT_GATE_TYPE_TRAP);
	idt_set_gate(19, isr19, IDT_GATE_TYPE_TRAP);
	idt_set_gate(20, isr20, IDT_GATE_TYPE_TRAP);
	idt_set_gate(21, isr21, IDT_GATE_TYPE_TRAP);
	idt_set_gate(22, isr22, IDT_GATE_TYPE_TRAP);
	idt_set_gate(23, isr23, IDT_GATE_TYPE_TRAP);
	idt_set_gate(24, isr24, IDT_GATE_TYPE_TRAP);
	idt_set_gate(25, isr25, IDT_GATE_TYPE_TRAP);
	idt_set_gate(26, isr26, IDT_GATE_TYPE_TRAP);
	idt_set_gate(27, isr27, IDT_GATE_TYPE_TRAP);
	idt_set_gate(28, isr28, IDT_GATE_TYPE_TRAP);
	idt_set_gate(29, isr29, IDT_GATE_TYPE_TRAP);
	idt_set_gate(30, isr30, IDT_GATE_TYPE_TRAP);
	idt_set_gate(31, isr31, IDT_GATE_TYPE_TRAP);

	/* remap PICs */
	port_outb(IDT_PIC0_CMD, IDT_PIC_CMD_ICW1);
	_wait();
	port_outb(IDT_PIC1_CMD, IDT_PIC_CMD_ICW1);
	_wait();

	port_outb(IDT_PIC0_DATA, 0x20); /* just after isrs */
	_wait();
	port_outb(IDT_PIC1_DATA, 0x28); /* after PIC0 */
	_wait();

	port_outb(IDT_PIC0_DATA, 0x04);
	_wait();
	port_outb(IDT_PIC1_DATA, 0x02);
	_wait();

	port_outb(IDT_PIC0_DATA, 0x01);
	_wait();
	port_outb(IDT_PIC1_DATA, 0x01);
	_wait();

	/* enable irqs */
	port_outb(IDT_PIC0_DATA, 0x00);
	_wait();
	port_outb(IDT_PIC1_DATA, 0x00);
	_wait();

	/* setup irqs */
	idt_set_gate(32, irq0, IDT_GATE_TYPE_INT);
	idt_set_gate(33, irq1, IDT_GATE_TYPE_INT);
	idt_set_gate(34, irq2, IDT_GATE_TYPE_INT);
	idt_set_gate(35, irq3, IDT_GATE_TYPE_INT);
	idt_set_gate(36, irq4, IDT_GATE_TYPE_INT);
	idt_set_gate(37, irq5, IDT_GATE_TYPE_INT);
	idt_set_gate(38, irq6, IDT_GATE_TYPE_INT);
	idt_set_gate(39, irq7, IDT_GATE_TYPE_INT);
	idt_set_gate(40, irq8, IDT_GATE_TYPE_INT);
	idt_set_gate(41, irq9, IDT_GATE_TYPE_INT);
	idt_set_gate(42, irq10, IDT_GATE_TYPE_INT);
	idt_set_gate(43, irq11, IDT_GATE_TYPE_INT);
	idt_set_gate(44, irq12, IDT_GATE_TYPE_INT);
	idt_set_gate(45, irq13, IDT_GATE_TYPE_INT);
	idt_set_gate(46, irq14, IDT_GATE_TYPE_INT);
	idt_set_gate(47, irq15, IDT_GATE_TYPE_INT);

	/* load idt */
	idt_desc.size = sizeof(idt) - 1;
	idt_desc.addr = (uint32_t)&idt;
	__asm__("lidt (%0)": : "r"(&idt_desc));
}

/* enable interrupts */
extern void idt_enable(void) {

	__asm__("sti");
}

/* set handler */
extern void idt_set_gate(uint32_t n, void *p, uint8_t tp) {

	idt[n].off0 = (uint32_t)p & 0xffff;
	idt[n].off1 = ((uint32_t)p >> 16) & 0xffff;
	idt[n].segsel = 0x8; /* supervisor code selector */
	idt[n].rsvd = 0;
	idt[n].type = tp & 0xf;
	idt[n].flags = IDT_GATE_FLAG_P | IDT_GATE_FLAG_DPLU;
}

/* main isr handler */
extern void idt_isr_handler(idt_regs_t *regs) {

	kprintf(LOG_FATAL, "[idt] Exception: %d (code: 0x%x)", regs->n_int, regs->err_code);
	kpanic(regs->n_int == 14? PANIC_CODE_FAULT: 0, "CPU exception", regs);
}

/* main irq handler */
extern void idt_irq_handler(idt_regs_t *regs) {

	if (isrs[regs->n_int] != NULL) isrs[regs->n_int](regs);

	/* end of interrupt command */
	if (regs->n_int >= 40 && regs->n_int < 48 && noeoi[regs->n_int-40])
		return;

	port_outb(IDT_PIC0_CMD, IDT_PIC_CMD_EOI);
	if (regs->n_int >= 40) port_outb(IDT_PIC1_CMD, IDT_PIC_CMD_EOI);
}

/* set irq callback */
extern void idt_set_irq_callback(uint32_t n, idt_isr_t isr) {

	if (n > 15) return;
	isrs[32 + n] = isr;
}

/* send eoi command to first pic */
extern void idt_send_eoi(void) {

	port_outb(IDT_PIC0_CMD, IDT_PIC_CMD_EOI);
	port_outb(IDT_PIC1_CMD, IDT_PIC_CMD_EOI);
}

/* disable automatic eoi command for irqs */
extern void idt_disable_irq_eoi(uint32_t n) {

	if (n < 8) noeoi[n] = true;
}
