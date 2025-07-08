/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * The code of ytosec, mtosec and rtc_get_time along
 * with the values in msec are largely derived from
 * musl libc (https://musl.libc.org).
 */
#include <kernel/types.h>
#include <kernel/idt.h>
#include <kernel/tty.h>
#include <kernel/io/port.h>
#include <kernel/driver/rtc.h>

#define RTC_RATE 1 /* 1 kHz (32768 >> (6-1)) */

uint64_t rtc_pass = 0; /* time to pass every irq */
uint64_t rtc_ns = 0; /* time nanosecond value */

/* seconds at start of each month */
static const uint64_t msec[12] = {
	0, /* January */
	31*86400, /* February */
	59*86400, /* March */
	90*86400, /* April */
	120*86400, /* May */
	151*86400, /* June */
	181*86400, /* July */
	212*86400, /* August */
	243*86400, /* September */
	273*86400, /* October */
	304*86400, /* November */
	334*86400, /* December */
};

/* default callback */
static void rtc_irq(idt_regs_t *regs) {

	rtc_ns += rtc_pass;
	rtc_enable_next_interrupt();
}

/* convert year to seconds */
static uint64_t ytosec(uint64_t year, bool *is_leap) {

	if (year < 1902) return 0;
	year -= 1900;

	if (year-2 <= 136) {

		uint64_t leaps = (year-68) >> 2;
		if (!((year-68)&3)) {
			leaps--;
			*is_leap = true;
		} else *is_leap = false;
		return 31536000 * (year - 70) + 86400 * leaps;
	}
	uint64_t cycles, centuries, leaps, rem;

	cycles = (year - 100) / 400;
	rem = (year - 100) % 400;
	if (rem < 0) {
		cycles--;
		rem += 400;
	}
	if (!rem) {
		*is_leap = true;
		centuries = 0;
		leaps = 0;
	} else {
		if (rem >= 200) {
			if (rem >= 300) centuries = 3, rem -= 300;
			else centuries = 2, rem -= 200;
		} else {
			if (rem >= 100) centuries = 1, rem -= 100;
			else centuries = 0;
		}
		if (!rem) {
			*is_leap = false;
			leaps = 0;
		} else {
			leaps = rem / 4;
			rem %= 4;
			*is_leap = !rem;
		}
	}

	leaps += 97 * cycles + 24 * centuries - (uint64_t)*is_leap;

	return (year - 100) * 31536000 + leaps * 86400 + 946684800 + 86400;
}

/* convert month to seconds */
static uint64_t mtosec(uint64_t month, bool is_leap) {

	uint64_t t = msec[month];
	if (is_leap && month >= 2) t += 86400;
	return t;
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

/* get time from register values */
#define BCD2INT(d) ((((d) >> 4) * 10) + ((d) & 0xf))

extern uint64_t rtc_get_time(rtc_cmos_regs_t *regs) {

	/* fix time values */
	uint64_t tm_sec = (uint64_t)regs->seconds;
	uint64_t tm_min = (uint64_t)regs->minutes;
	uint64_t tm_hour = (uint64_t)regs->hours;
	uint64_t tm_mday = (uint64_t)regs->dayofmonth;
	uint64_t tm_mon = (uint64_t)regs->month;
	uint64_t tm_year = (uint64_t)regs->year;

	if (!(regs->status[1] & 0x1)) {
		tm_sec = BCD2INT(tm_sec);
		tm_min = BCD2INT(tm_min);
		tm_hour = BCD2INT(tm_hour);
		tm_mday = BCD2INT(tm_mday);
		tm_mon = BCD2INT(tm_mon);
		tm_year = BCD2INT(tm_year);
	}
	tm_mon--;
	tm_year += 2000;

	if (!(regs->status[1] & 0x2)) {
		if (tm_hour & 0x80) tm_hour = (tm_hour & 0x7f) + 12;
		tm_hour--;
	}

	/* convert to seconds */
	bool is_leap;
	uint64_t t = ytosec(tm_year, &is_leap);
	t += mtosec(tm_mon, is_leap);
	t += 86400 * (tm_mday-1);
	t += 3600 * tm_hour;
	t += 60 * tm_min;
	t += tm_sec;
	return t;
}
