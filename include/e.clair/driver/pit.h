#ifndef ECLAIR_DRIVER_PIT_H
#define ECLAIR_DRIVER_PIT_H

#include <e.clair/types.h>

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

/* functions */
extern void pit_init(void); /* initialize pit */
extern void pit_set_mode(uint8_t mode); /* set operating mode */
extern void pit_set_channel(uint8_t ch, uint8_t val); /* set value of channel */

#endif /* ECLAIR_DRIVER_PIT_H */
