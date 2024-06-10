#ifndef ECLAIR_TTY_H
#define ECLAIR_TTY_H

#include <stddef.h>
#include <stdint.h>

/* variables to set */
extern size_t tty_width, tty_height;
extern uint8_t *tty_mem;

/* functions */
extern void tty_init(void); /* initialize */
extern void tty_clear(void); /* clear screen data */
extern void tty_write(void *buf, size_t n); /* write characters */
extern void tty_print(const char *s); /* print string */
extern void tty_printc(char c); /* print character */
extern void tty_printi(int i); /* print integer */
extern void tty_printh(uint32_t h); /* print hexadecimal number */
extern void tty_printf(const char *fmt, ...); /* print formatted */
extern void tty_printnl(void); /* print newline */

#endif /* ECLAIR_TTY_H */
