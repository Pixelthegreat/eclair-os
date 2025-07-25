/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_DRIVER_PIT_H
#define ECLAIR_DRIVER_PIT_H

#include <kernel/types.h>
#include <kernel/idt.h>

#define PIT_FREQ(x) (1193182 / (x))
#define PIT_IRQ 0

/* io ports */
#define PIT_PORT_CHANNEL0 0x40
#define PIT_PORT_CHANNEL1 0x41
#define PIT_PORT_CHANNEL2 0x42
#define PIT_PORT_MODE 0x43

/* modes */
#define PIT_CHANNEL0 0
#define PIT_CHANNEL1 1
#define PIT_CHANNEL2 2

#define PIT_ACCESS_LATCH 0
#define PIT_ACCESS_LO 1
#define PIT_ACCESS_HI 2
#define PIT_ACCESS_HILO 3

#define PIT_MODE_INT 0
#define PIT_MODE_ONE 1
#define PIT_MODE_RATE 2 /* also 6 */
#define PIT_MODE_SQUARE 3 /* also 7 */
#define PIT_MODE_SOFT_STROBE 4
#define PIT_MODE_HARD_STROBE 5

#define PIT_MODE_BCD 1

#define PIT_CHANNEL(c) ((c) << 6)
#define PIT_ACCESS(a) ((a) << 4)
#define PIT_MODE(m) ((m) << 1)

#define PIT_COMMAND(c, a, m) (PIT_CHANNEL(c) | PIT_ACCESS(a) | PIT_MODE(m))

typedef void (*pit_callback_t)(idt_regs_t *); /* timer interrupt callback */

/* functions */
extern void pit_init(void); /* initialize pit */
extern void pit_delay(uint32_t div); /* delay */
extern void pit_delay_ms(uint32_t ms); /* delay milliseconds */
extern void pit_set_mode(uint8_t mode); /* set operating mode */
extern void pit_set_channel(uint8_t ch, uint8_t val); /* set value of channel */
extern void pit_set_callback(pit_callback_t cb); /* set timer callback */

#endif /* ECLAIR_DRIVER_PIT_H */
