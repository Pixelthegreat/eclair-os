#ifndef ECLAIR_DRIVER_VGACON_H
#define ECLAIR_DRIVER_VGACON_H

#include <kernel/types.h>

/* functions */
extern void vgacon_init(void); /* base init */
extern void vgacon_set_tty(void); /* set kernel tty device */
extern void vgacon_init_tty(void); /* init and set tty device */
extern kssize_t vgacon_read(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf); /* read */
extern kssize_t vgacon_write(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf); /* write */

#endif /* ECLAIR_DRIVER_VGACON_H */
