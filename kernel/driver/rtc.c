#include <kernel/types.h>
#include <kernel/idt.h>
#include <kernel/tty.h>
#include <kernel/io/port.h>
#include <kernel/driver/rtc.h>

#define RTC_RATE 1 /* 1 kHz (32768 >> (6-1)) */

uint64_t rtc_pass = 0; /* time to pass every irq */
uint64_t rtc_ns = 0; /* time nanosecond value */

/* default callback */
static void rtc_irq(idt_regs_t *regs) {

	rtc_ns += rtc_pass;
	rtc_enable_next_interrupt();
}

/* initialize rtc */
extern void rtc_init(void) {

	asm("cli");

	rtc_set_callback(rtc_irq);

	/* enable interrupt */
	port_outb(RTC_PORT_INDEX, 0x8b);
	uint8_t val = port_inb(RTC_PORT_DATA);

	port_outb(RTC_PORT_INDEX, 0x8b);
	port_outb(RTC_PORT_DATA, val | 0x40);

	/* set interrupt rate */
	port_outb(RTC_PORT_INDEX, 0x8a); /* disable nmi */
	val = port_inb(RTC_PORT_DATA);

	port_outb(RTC_PORT_INDEX, 0x8a);
	port_outb(RTC_PORT_DATA, (val & 0xf0) | (RTC_RATE & 0xf));

	/* get amount of time that will pass */
	rtc_pass = 1000000000 / (32768 >> (RTC_RATE-1));
	
	asm("sti");
}

/* set callback */
extern void rtc_set_callback(rtc_callback_t cb) {

	idt_set_irq_callback(8, cb);
}

/* get register values */
extern void rtc_get_registers(rtc_cmos_regs_t *regs) {

	regs->seconds = rtc_read_cmos_reg(RTC_CMOS_REG_SECONDS);
	regs->minutes = rtc_read_cmos_reg(RTC_CMOS_REG_MINUTES);
	regs->hours = rtc_read_cmos_reg(RTC_CMOS_REG_HOURS);
	regs->weekday = rtc_read_cmos_reg(RTC_CMOS_REG_WEEKDAY);
	regs->dayofmonth = rtc_read_cmos_reg(RTC_CMOS_REG_DAYOFMONTH);
	regs->month = rtc_read_cmos_reg(RTC_CMOS_REG_MONTH);
	regs->year = rtc_read_cmos_reg(RTC_CMOS_REG_YEAR);
	regs->century = rtc_read_cmos_reg(RTC_CMOS_REG_CENTURY);
	regs->status[0] = rtc_read_cmos_reg(RTC_CMOS_REG_STATUS_A);
	regs->status[1] = rtc_read_cmos_reg(RTC_CMOS_REG_STATUS_B);
}
