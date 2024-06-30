#include <e.clair/types.h>
#include <e.clair/io/port.h>
#include <e.clair/tty.h>
#include <e.clair/driver/ps2.h>

/* initialize */
extern void ps2_init(void) {

	/* disable ps2 ports */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_DISABLE_P1);

	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_DISABLE_P2);

	/* flush output buffer */
	(void)port_inb(PS2_PORT_DATA);

	/* disable configuration byte flags */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_READ_B0);
	ps2_wait_read();
	uint8_t fl = port_inb(PS2_PORT_DATA);

	fl &= ~(PS2_CONFIG_P1_INT | PS2_CONFIG_P2_INT | PS2_CONFIG_P1_TRANSLATE);

	int p2_pres = fl & PS2_CONFIG_P2_CLOCK; /* two ports */

	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_B0);
	ps2_wait_write();
	port_outb(PS2_PORT_DATA, fl);

	/* self test */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_TEST_CTRL);
	ps2_wait_read();
	if (port_inb(PS2_PORT_DATA) != 0x55) {

		tty_printf("[ps/2 8042] self test failed\n");
		return;
	}

	/* test first port */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_TEST_P1);
	ps2_wait_read();
	if (port_inb(PS2_PORT_DATA) != 0) {

		tty_printf("[ps/2 8042] port 1 test failed\n");
		return;
	}

	/* test second port */
	if (p2_pres) {

		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_TEST_P2);
		ps2_wait_read();
		if (port_inb(PS2_PORT_DATA) != 0) {

			tty_printf("[ps/2 8042] port 2 test failed\n");
			return;
		}
	}

	/* reenable devices */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_ENABLE_P1);

	if (p2_pres) {
		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_ENABLE_P2);
	}

	/* reset devices */
	if (ps2_reset_device(0) < 0) {

		tty_printf("[ps/2 8042] failed to reset device on port 1\n");
		return;
	}

	if (p2_pres && ps2_reset_device(1) < 0) {

		tty_printf("[ps/2 8042] failed to reset device on port 2\n");
		return;
	}

	/* set handlers */
	idt_set_irq_callback(1, ps2_irq1);
	if (p2_pres) idt_set_irq_callback(12, ps2_irq12);

	/* enable interrupts */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_READ_B0);
	ps2_wait_read();
	fl = port_inb(PS2_PORT_DATA);

	fl |= PS2_CONFIG_P1_INT;
	if (p2_pres) fl |= PS2_CONFIG_P2_INT;

	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_B0);
	ps2_wait_write();
	port_outb(PS2_PORT_DATA, fl);
}

/* wait for read */
extern void ps2_wait_read(void) {

	uint8_t stat;
	while (!((stat = port_inb(PS2_PORT_CMD_STAT)) & PS2_STATUS_OUT_BUF));
}

/* wait for write */
extern void ps2_wait_write(void) {

	uint8_t stat;
	while ((stat = port_inb(PS2_PORT_CMD_STAT)) & PS2_STATUS_IN_BUF);
}

/* reset device */
extern int ps2_reset_device(int d) {

	/* switch to port 2 */
	if (d) {
		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_NEXT_P2_INB);
	}

	/* write reset command */
	uint8_t res = PS2_DEV_CMD_RES_RESEND;
	while (res == PS2_DEV_CMD_RES_RESEND) {

		ps2_wait_write();
		port_outb(PS2_PORT_DATA, PS2_DEV_CMD_RESET);

		ps2_wait_read();
		res = port_inb(PS2_PORT_DATA);
	}

	/* expects ack (acknowledged) followed by success */
	if (res != PS2_DEV_CMD_RES_ACK) return -1;

	ps2_wait_read();
	res = port_inb(PS2_PORT_DATA);
	if (res != PS2_DEV_CMD_RES_PASS) return -1;

	/* success */
	return 0;
}

/* get device type */
extern int ps2_get_device_type(int d) {

	if (d) {
		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_NEXT_P2_INB);
	}

	/* disable scanning */
	uint8_t res = PS2_DEV_CMD_RES_RESEND;
	while (res == PS2_DEV_CMD_RES_RESEND) {

		ps2_wait_write();
		port_outb(PS2_PORT_DATA, PS2_DEV_CMD_DISABLE_SCAN);

		ps2_wait_read();
		res = port_inb(PS2_PORT_DATA);
	}
	if (res != PS2_DEV_CMD_RES_ACK) return -1;
	return 0;
}

/* irq1 for first ps/2 device */
extern void ps2_irq1(idt_regs_t *regs) {

	(void)port_inb(PS2_PORT_DATA);
}

/* irq12 for second ps/2 device */
extern void ps2_irq12(idt_regs_t *regs) {

	(void)port_inb(PS2_PORT_DATA);
}
