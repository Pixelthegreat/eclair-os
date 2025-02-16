#ifndef ECLAIR_TTY_H
#define ECLAIR_TTY_H

#include <kernel/types.h>
#include <kernel/vfs/fs.h>

/* functions */
extern void tty_init(void); /* initialize */
extern void tty_add_device(fs_node_t *devn); /* add character device */
extern fs_node_t *tty_get_device(int i); /* get character device */
extern void tty_write(void *buf, size_t n); /* write characters */
extern void tty_print(const char *s); /* print string */
extern void tty_printi(int i); /* print integer */
extern void tty_printh(uint32_t h); /* print hexadecimal number */
extern void tty_printf(const char *fmt, ...); /* print formatted */
extern void tty_printnl(void); /* print newline */

#endif /* ECLAIR_TTY_H */
