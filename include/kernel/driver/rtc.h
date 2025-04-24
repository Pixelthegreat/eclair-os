#ifndef ECLAIR_DRIVER_RTC_H
#define ECLAIR_DRIVER_RTC_H

#include <kernel/types.h>
#include <kernel/io/port.h>

/* io ports */
#define RTC_PORT_INDEX 0x70
#define RTC_PORT_DATA 0x71

/* cmos rtc registers */
#define RTC_CMOS_REG_SECONDS 0x00
#define RTC_CMOS_REG_MINUTES 0x02
#define RTC_CMOS_REG_HOURS 0x04
#define RTC_CMOS_REG_WEEKDAY 0x06
#define RTC_CMOS_REG_DAYOFMONTH 0x07
#define RTC_CMOS_REG_MONTH 0x08
#define RTC_CMOS_REG_YEAR 0x09
#define RTC_CMOS_REG_CENTURY 0x32
#define RTC_CMOS_REG_STATUS_A 0x0a
#define RTC_CMOS_REG_STATUS_B 0x0b

typedef struct rtc_cmos_regs {
	uint8_t seconds,
		minutes,
		hours,
		weekday,
		dayofmonth,
		month,
		year,
		century,
		status[2];
} rtc_cmos_regs_t;

typedef void (*rtc_callback_t)(idt_regs_t *); /* callback for rtc */

extern uint64_t rtc_pass; /* time that passes every irq */
extern uint64_t rtc_ns; /* nanoseconds, irq must update */

/* functions */
extern void rtc_init(void); /* initialize rtc */
extern void rtc_set_callback(rtc_callback_t _cb); /* set callback */
extern void rtc_get_registers(rtc_cmos_regs_t *regs); /* get register values */
extern uint64_t rtc_get_time(rtc_cmos_regs_t *regs); /* get time from register values */

/* enable next interrupt to occur */
static inline void rtc_enable_next_interrupt(void) {

	port_outb(RTC_PORT_INDEX, 0x0c);
	(void)port_inb(RTC_PORT_DATA);
}

/* read a cmos register */
static inline uint8_t rtc_read_cmos_reg(uint8_t reg) {

	port_outb(RTC_PORT_INDEX, reg | 0x80);
	return port_inb(RTC_PORT_DATA);
}

#endif /* ECLAIR_DRIVER_RTC_H */
