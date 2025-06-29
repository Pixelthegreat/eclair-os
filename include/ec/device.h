/*
 * Data for ioctl calls
 */
#ifndef EC_DEVICE_H
#define EC_DEVICE_H

/*
 * == TTY Settings ==
 */

/*
 * Set or get cursor updates
 *   arg:
 *     0 = Get cursor update state
 *     1 = Flip cursor update state
 */
#define ECIO_TTY_CURSOR 0x02

#endif /* EC_DEVICE_H */
