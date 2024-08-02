#ifndef ECLAIR_DRIVER_RTC_H
#define ECLAIR_DRIVER_RTC_H

#include <e.clair/types.h>
#include <e.clair/io/port.h>

/* io ports */
#define RTC_PORT_INDEX 0x70
#define RTC_PORT_DATA 0x71

typedef void (*rtc_callback_t)(idt_regs_t *); /* callback for rtc */

extern uint64_t rtc_pass; /* time that passes every irq */
extern uint64_t rtc_ns; /* nanoseconds, irq must update */

/* functions */
extern void rtc_init(void); /* initialize rtc */
extern void rtc_set_callback(rtc_callback_t _cb); /* set callback */

static inline void rtc_enable_next_interrupt(void) {

	port_outb(RTC_PORT_INDEX, 0x0c);
	(void)port_inb(RTC_PORT_DATA);
}

#endif /* ECLAIR_DRIVER_RTC_H */
