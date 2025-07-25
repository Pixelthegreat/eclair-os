/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/io/port.h>
#include <kernel/vfs/devfs.h>
#include <kernel/driver/device.h>
#include <ec/device.h>
#include <kernel/driver/ps2.h>

enum  {
	DEV_UNKNOWN = 0,
	DEV_KEYBOARD,
	DEV_MOUSE,

	DEV_COUNT,
};

static device_t *(*creates[DEV_COUNT])(const char *) = {
	device_terminal_new, /* used here as dummy device */
	device_keyboard_new,
	device_mouse_new,
};
static const char *names[DEV_COUNT] = {
	"PS/2 Device",
	"PS/2 Keyboard",
	"PS/2 Mouse",
};

static bool wait_int = true; /* ignore interrupts */
static device_t *bus = NULL; /* device bus */
static device_t *dev_p0 = NULL, *dev_p1 = NULL; /* devices */
static bool rel = false; /* key was released */
static bool other = false; /* key was pressed */
static uint8_t mbytes[3]; /* mouse bytes */
static size_t nmbytes; /* number of mouse bytes */
static fs_node_t *node_p0 = NULL, *node_p1 = NULL; /* fs nodes */

static int dev_p0_type, dev_p1_type; /* device types */

static uint8_t ps2_send_command(uint8_t cmd);

/* scancode lookup tables */
static uint32_t scancodes2[256];
static uint32_t scancodes2_other[256];

/* translate scancode */
static uint32_t translate_code(uint8_t b) {

	if (b == 0xf0) { /* key was released */

		rel = true;
		return 0;
	}
	if (b == 0xe0) { /* other key */

		other = true;
		return 0;
	}

	uint32_t key = other? scancodes2_other[b]: scancodes2[b];
	if (rel) key |= ECK_RELEASE;

	rel = false;
	other = false;
	return key;
}

/* keyboard ioctl */
static int ioctl_kbd(fs_node_t *node, int op, uintptr_t arg) {

	switch (op) {
		case ECIO_INP_GETEVENT:
			return device_keyboard_getkey(node->impl? dev_p1: dev_p0);
		case ECIO_INP_FLUSH:
			device_keyboard_flushkeys(node->impl? dev_p1: dev_p0);
			return 0;
		default:
			return -ENOSYS;
	}
}

/* mouse ioctl */
static int ioctl_mus(fs_node_t *node, int op, uintptr_t arg) {

	switch (op) {
		case ECIO_INP_GETEVENT:
			ec_msevent_t *event = (ec_msevent_t *)arg;
			if (!event) return -EINVAL;

			device_mouse_event_t *dmev = device_mouse_getev(node->impl? dev_p1: dev_p0);
			if (!dmev) return 1;

			event->x = dmev->x;
			event->y = dmev->y;
			for (int i = 0; i < ECB_COUNT; i++)
				event->state[i] = dmev->st[i];
			return 0;
		case ECIO_INP_FLUSH:
			device_mouse_flushevs(node->impl? dev_p1: dev_p0);
			return 0;
		default:
			return -ENOSYS;
	}
}

/* fill scancode sets with appropriate translations */
static void ps2_fill_scancode_sets(void) {

	scancodes2[0x01] = ECK_F9;
	scancodes2[0x03] = ECK_F5;
	scancodes2[0x04] = ECK_F3;
	scancodes2[0x05] = ECK_F1;
	scancodes2[0x06] = ECK_F6;
	scancodes2[0x07] = ECK_F12;
	scancodes2[0x09] = ECK_F10;
	scancodes2[0x0a] = ECK_F8;
	scancodes2[0x0b] = ECK_F6;
	scancodes2[0x0c] = ECK_F4;

	scancodes2[0x0d] = ECK_TAB;
	scancodes2[0x0e] = ECK_BACKTICK;
	scancodes2[0x11] = ECK_LEFT_ALT;
	scancodes2[0x12] = ECK_LEFT_SHIFT;
	scancodes2[0x14] = ECK_LEFT_CONTROL;

	scancodes2[0x15] = ECK_Q;
	scancodes2[0x16] = ECK_1;
	scancodes2[0x1a] = ECK_Z;
	scancodes2[0x1b] = ECK_S;
	scancodes2[0x1c] = ECK_A;
	scancodes2[0x1d] = ECK_W;
	scancodes2[0x1e] = ECK_2;
	scancodes2[0x21] = ECK_C;
	scancodes2[0x22] = ECK_X;
	scancodes2[0x23] = ECK_D;
	scancodes2[0x24] = ECK_E;
	scancodes2[0x25] = ECK_4;
	scancodes2[0x26] = ECK_3;
	scancodes2[0x29] = ECK_SPACE;
	scancodes2[0x2a] = ECK_V;
	scancodes2[0x2b] = ECK_F;
	scancodes2[0x2c] = ECK_T;
	scancodes2[0x2d] = ECK_R;
	scancodes2[0x2e] = ECK_5;
	scancodes2[0x31] = ECK_N;
	scancodes2[0x32] = ECK_B;
	scancodes2[0x33] = ECK_H;
	scancodes2[0x34] = ECK_G;
	scancodes2[0x35] = ECK_Y;
	scancodes2[0x36] = ECK_6;
	scancodes2[0x3a] = ECK_M;
	scancodes2[0x3b] = ECK_J;
	scancodes2[0x3c] = ECK_U;
	scancodes2[0x3d] = ECK_7;
	scancodes2[0x3e] = ECK_8;
	scancodes2[0x41] = ECK_COMMA;
	scancodes2[0x42] = ECK_K;
	scancodes2[0x43] = ECK_I;
	scancodes2[0x44] = ECK_O;
	scancodes2[0x45] = ECK_0;
	scancodes2[0x46] = ECK_9;
	scancodes2[0x49] = ECK_DOT;
	scancodes2[0x4a] = ECK_SLASH;
	scancodes2[0x4b] = ECK_L;
	scancodes2[0x4c] = ECK_SEMICOLON;
	scancodes2[0x4d] = ECK_P;
	scancodes2[0x4e] = ECK_MINUS;
	scancodes2[0x52] = ECK_QUOTE;
	scancodes2[0x54] = ECK_LEFT_BRACKET;
	scancodes2[0x55] = ECK_EQUALS;

	scancodes2[0x58] = ECK_CAPS_LOCK;
	scancodes2[0x59] = ECK_RIGHT_SHIFT;
	scancodes2[0x5a] = ECK_RETURN;
	scancodes2[0x5b] = ECK_RIGHT_BRACKET;
	scancodes2[0x5d] = ECK_BACKSLASH;
	scancodes2[0x66] = ECK_BACKSPACE;

	scancodes2[0x69] = ECK_NUMPAD_1;
	scancodes2[0x6b] = ECK_NUMPAD_4;
	scancodes2[0x6c] = ECK_NUMPAD_7;
	scancodes2[0x70] = ECK_NUMPAD_0;
	scancodes2[0x71] = ECK_NUMPAD_DOT;
	scancodes2[0x72] = ECK_NUMPAD_2;
	scancodes2[0x73] = ECK_NUMPAD_5;
	scancodes2[0x74] = ECK_NUMPAD_6;
	scancodes2[0x75] = ECK_NUMPAD_8;
	scancodes2[0x76] = ECK_ESCAPE;
	scancodes2[0x77] = ECK_NUM_LOCK;
	scancodes2[0x78] = ECK_F11;
	scancodes2[0x79] = ECK_NUMPAD_PLUS;
	scancodes2[0x7a] = ECK_NUMPAD_3;
	scancodes2[0x7b] = ECK_NUMPAD_MINUS;
	scancodes2[0x7c] = ECK_NUMPAD_ASTERISK;
	scancodes2[0x7d] = ECK_NUMPAD_9;

	/* other scancodes */
	scancodes2_other[0x11] = ECK_RIGHT_ALT;
	scancodes2_other[0x14] = ECK_RIGHT_CONTROL;
	scancodes2_other[0x4a] = ECK_NUMPAD_SLASH;
	scancodes2_other[0x5a] = ECK_NUMPAD_ENTER;

	scancodes2_other[0x69] = ECK_END;
	scancodes2_other[0x6b] = ECK_LEFT;
	scancodes2_other[0x6c] = ECK_HOME;
	scancodes2_other[0x70] = ECK_INSERT;
	scancodes2_other[0x71] = ECK_DELETE;
	scancodes2_other[0x72] = ECK_DOWN;
	scancodes2_other[0x74] = ECK_RIGHT;
	scancodes2_other[0x75] = ECK_UP;
	scancodes2_other[0x7a] = ECK_PAGE_DOWN;
	scancodes2_other[0x7d] = ECK_PAGE_UP;
}

/* initialize */
extern void ps2_init(void) {

	ps2_fill_scancode_sets();

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

		kprintf(LOG_WARNING, "[ps/2 8042] Self test failed");
		return;
	}

	/* test first port */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_TEST_P1);
	ps2_wait_read();
	if (port_inb(PS2_PORT_DATA) != 0) {

		kprintf(LOG_WARNING, "[ps/2 8042] Port 1 test failed");
		return;
	}

	/* test second port */
	if (p2_pres) {

		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_TEST_P2);
		ps2_wait_read();
		if (port_inb(PS2_PORT_DATA) != 0) {

			kprintf(LOG_WARNING, "[ps/2 8042] Port 2 test failed");
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

		kprintf(LOG_WARNING, "[ps/2 8042] Failed to reset device on port 1");
		return;
	}

	if (p2_pres && ps2_reset_device(1) < 0) {

		kprintf(LOG_WARNING, "[ps/2 8042] Failed to reset device on port 2");
		p2_pres = 0;
	}

	/* detect device types */
	dev_p0_type = ps2_get_device_type(0);
	if (p2_pres) dev_p1_type = ps2_get_device_type(1);

	/* TODO: Actually fix ps/2 device type detection issue on real hw */
	if (dev_p0_type < 0) dev_p0_type = DEV_KEYBOARD;
	if (dev_p1_type < 0) dev_p1_type = DEV_KEYBOARD;

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

	/* create devices */
	bus = device_bus_new("PS/2 8042+");

	dev_p0 = creates[dev_p0_type](names[dev_p0_type]);
	device_bus_add(bus, dev_p0);
	kprintf(LOG_INFO, "[ps/2 8042] Detected first PS/2 device");

	if (p2_pres) {
		
		dev_p1 = creates[dev_p1_type](names[dev_p1_type]);
		device_bus_add(bus, dev_p1);
		kprintf(LOG_INFO, "[ps/2 8042] Detected second PS/2 device");
	}

	/* disable ignoring of interrupts */
	wait_int = false;
}

/* add devices */
extern void ps2_init_devfs(void) {

	if (dev_p0_type) {

		node_p0 = fs_node_new(NULL, FS_CHARDEVICE);
		node_p0->mask = 0644;
		node_p0->impl = 0;
		node_p0->ioctl = (dev_p0_type == DEV_KEYBOARD)? ioctl_kbd: ioctl_mus;
		devfs_add_node(dev_p0_type == DEV_KEYBOARD? "kbd": "mus", node_p0);
	}
	if (dev_p1_type) {

		node_p1 = fs_node_new(NULL, FS_CHARDEVICE);
		node_p1->mask = 0644;
		node_p1->impl = 1;
		node_p1->ioctl = (dev_p1_type == DEV_KEYBOARD)? ioctl_kbd: ioctl_mus;
		devfs_add_node(dev_p1_type == DEV_KEYBOARD? "kbd": "mus", node_p1);
	}
}

/* wait for read */
#define PS2_TIMEOUT 131072
extern void ps2_wait_read(void) {

	uint8_t stat;
	for (int i = 0; !((stat = port_inb(PS2_PORT_CMD_STAT)) & PS2_STATUS_OUT_BUF) && i < PS2_TIMEOUT; i++);
}

/* wait for write */
extern void ps2_wait_write(void) {

	uint8_t stat;
	for (int i = 0; ((stat = port_inb(PS2_PORT_CMD_STAT)) & PS2_STATUS_IN_BUF) && i < PS2_TIMEOUT; i++);
}

/* flush input buffer */
extern void ps2_flush(void) {

	for (int i = 0; i < 4; i++) {

		ps2_wait_read();
		(void)port_inb(PS2_PORT_DATA);
	}
}

/* send ps2 command */
static uint8_t ps2_send_command(uint8_t cmd) {

	uint8_t res = PS2_DEV_CMD_RES_RESEND;
	while (res == PS2_DEV_CMD_RES_RESEND) {

		ps2_wait_write();
		port_outb(PS2_PORT_DATA, cmd);

		ps2_wait_read();
		res = port_inb(PS2_PORT_DATA);
	}
	return res;
}

/* reset device */
extern int ps2_reset_device(int d) {

	/* switch to port 2 */
	if (d) {
		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_NEXT_P2_INB);
	}

	/* send reset command */
	uint8_t res = ps2_send_command(PS2_DEV_CMD_RESET);

	/* expects ack (acknowledged) followed by success */
	if (res != PS2_DEV_CMD_RES_ACK) {

		ps2_flush();
		return -1;
	}

	ps2_wait_read();
	res = port_inb(PS2_PORT_DATA);
	if (res != PS2_DEV_CMD_RES_PASS) {

		ps2_flush();
		return -1;
	}

	ps2_wait_read();
	(void)port_inb(PS2_PORT_DATA);
	ps2_wait_read();
	(void)port_inb(PS2_PORT_DATA);

	/* success */
	return 0;
}

/* get device type */
extern int ps2_get_device_type(int d) {

	if (d) {
		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_NEXT_P2_INB);
	}

	uint8_t res = ps2_send_command(PS2_DEV_CMD_DISABLE_SCAN);
	if (res != PS2_DEV_CMD_RES_ACK)
		return -1;

	if (d) {
		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_NEXT_P2_INB);
	}

	res = ps2_send_command(PS2_DEV_CMD_IDENTIFY);
	if (res != PS2_DEV_CMD_RES_ACK)
		return -1;

	/* get device type bytes */
	ps2_wait_read();
	uint8_t hi = port_inb(PS2_PORT_DATA);
	ps2_wait_read();
	uint8_t lo = port_inb(PS2_PORT_DATA);

	/* reenable scanning */
	if (d) {
		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_WRITE_NEXT_P2_INB);
	}

	res = ps2_send_command(PS2_DEV_CMD_ENABLE_SCAN);
	if (res != PS2_DEV_CMD_RES_ACK)
		return -1;

	if (hi == PS2_DEV_TYPE_KBD) return DEV_KEYBOARD;
	else if (hi == PS2_DEV_TYPE_MOUSE) return DEV_MOUSE;

	return DEV_UNKNOWN;
}

/* keyboard irq function */
static void ps2_irqkbd(idt_regs_t *regs, int d) {

	device_t *dev = d? dev_p1: dev_p0;

	/* get scancode */
	uint8_t b = port_inb(PS2_PORT_DATA);

	uint32_t key = translate_code(b);
	if (!key) return;

	/* ignore */
	if (wait_int) return;

	/* add to input buffer */
	if (!dev) return;
	device_keyboard_putkey(dev, (int)key);
}

/* mouse irq function */
static void ps2_irqmouse(idt_regs_t *regs, int d) {

	device_t *dev = d? dev_p1: dev_p0;

	mbytes[nmbytes++] = port_inb(PS2_PORT_DATA);
	if (nmbytes < 3) return;
	nmbytes = 0;

	uint8_t b = mbytes[0];
	uint8_t rx = mbytes[1];
	uint8_t ry = mbytes[2];

	if (b & (PS2_MOUSE_FLAG_XO | PS2_MOUSE_FLAG_YO))
		return;

	/* fix x and y motion vector */
	int x = (int)rx;
	int y = (int)ry;

	if (x) x |= (b & PS2_MOUSE_FLAG_XS)? ~0xff: 0;
	if (y) y |= (b & PS2_MOUSE_FLAG_YS)? ~0xff: 0;

	y = -y;

	/* push event */
	device_mouse_event_t ev = {
		.x = x, .y = y,
		.st = {false, false, false},
	};
	if (b & PS2_MOUSE_FLAG_BL) ev.st[ECB_LEFT] = true;
	if (b & PS2_MOUSE_FLAG_BM) ev.st[ECB_MIDDLE] = true;
	if (b & PS2_MOUSE_FLAG_BR) ev.st[ECB_RIGHT] = true;

	device_mouse_putev(dev, &ev);
}

/* other irq function */
static void ps2_irqnone(idt_regs_t *regs, int d) {

	(void)port_inb(PS2_PORT_DATA);
}

/* irq1 for first ps/2 device */
extern void ps2_irq1(idt_regs_t *regs) {

	if (dev_p0_type == DEV_KEYBOARD) ps2_irqkbd(regs, 0);
	else if (dev_p0_type == DEV_MOUSE) ps2_irqmouse(regs, 0);
	else ps2_irqnone(regs, 0);
}

/* irq12 for second ps/2 device */
extern void ps2_irq12(idt_regs_t *regs) {

	if (dev_p1_type == DEV_KEYBOARD) ps2_irqkbd(regs, 1);
	else if (dev_p1_type == DEV_MOUSE) ps2_irqmouse(regs, 1);
	else ps2_irqnone(regs, 1);
}
