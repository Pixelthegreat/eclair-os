#ifndef ECLAIR_DRIVER_PS2_H
#define ECLAIR_DRIVER_PS2_H

#include <e.clair/types.h>
#include <e.clair/idt.h>

/* ports for ps/2 8042 controller */
#define PS2_PORT_DATA 0x60
#define PS2_PORT_CMD_STAT 0x64 /* command on write, status on read */

/* status flags */
#define PS2_STATUS_OUT_BUF 0x1
#define PS2_STATUS_IN_BUF 0x2
#define PS2_STATUS_SYS 0x4
#define PS2_STATUS_CMD 0x8
#define PS2_STATUS_TIME_OUT 0x40
#define PS2_STATUS_PARITY 0x80

/* controller commands */
#define PS2_CMD_READ_B0 0x20
#define PS2_CMD_READ_BYTE 0x21
#define PS2_CMD_WRITE_B0 0x60
#define PS2_CMD_WRITE_BYTE 0x61
#define PS2_CMD_DISABLE_P2 0xa7
#define PS2_CMD_ENABLE_P2 0xa8
#define PS2_CMD_TEST_P2 0xa9
#define PS2_CMD_TEST_CTRL 0xaa
#define PS2_CMD_TEST_P1 0xab
#define PS2_CMD_DIAG_DUMP 0xac
#define PS2_CMD_DISABLE_P1 0xad
#define PS2_CMD_ENABLE_P1 0xae
#define PS2_CMD_READ_CTRL_INP 0xc0
/* 0xc1 */
/* 0xc2 */
#define PS2_CMD_READ_CTRL_OUTP 0xd0
#define PS2_CMD_WRITE_NEXT_OUTP 0xd1
#define PS2_CMD_WRITE_NEXT_P1_OUTB 0xd2
#define PS2_CMD_WRITE_NEXT_P2_OUTB 0xd3
#define PS2_CMD_WRITE_NEXT_P2_INB 0xd4
#define PS2_CMD_PULSE_OUT_LINE 0xf0

/* controller configuration byte flags */
#define PS2_CONFIG_P1_INT 0x1
#define PS2_CONFIG_P2_INT 0x2
#define PS2_CONFIG_SYS 0x4
#define PS2_CONFIG_P1_CLOCK 0x10
#define PS2_CONFIG_P2_CLOCK 0x20
#define PS2_CONFIG_P1_TRANSLATE 0x40

/* controller output port flags */
#define PS2_OUTPORT_SYS_RESET 0x1
#define PS2_OUTPORT_A20 0x2
#define PS2_OUTPORT_P2_CLOCK 0x4
#define PS2_OUTPORT_P2_DATA 0x8
#define PS2_OUTPORT_OUTB_FULL_P1 0x10
#define PS2_OUTPORT_OUTB_FULL_P2 0x20
#define PS2_OUTPORT_P1_CLOCK 0x40
#define PS2_OUTPORT_P1_DATA 0x80

/* ps2 device commands */
#define PS2_DEV_CMD_RESEND 0xfe
#define PS2_DEV_CMD_RESET 0xff

/* device command results */
#define PS2_DEV_CMD_RES_ERR0 0x00
#define PS2_DEV_CMD_RES_PASS 0xaa
#define PS2_DEV_CMD_RES_ECHO 0xee
#define PS2_DEV_CMD_RES_ACK 0xfa
#define PS2_DEV_CMD_RES_RESEND 0xfe
#define PS2_DEV_CMD_RES_ERR1 0xff

/* functions */
extern void ps2_init(void); /* initialize */
extern void ps2_wait_read(void); /* wait for read */
extern void ps2_wait_write(void); /* wait for write */
extern int ps2_reset_device(int d); /* reset device */
extern void ps2_irq1(idt_regs_t *regs); /* irq1 for first ps/2 device */
extern void ps2_irq12(idt_regs_t *regs); /* irq12 for second ps/2 device */

#endif /* ECLAIR_DRIVER_PS2_H */
