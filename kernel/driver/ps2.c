#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/io/port.h>
#include <kernel/vfs/devfs.h>
#include <kernel/driver/device.h>
#include <kernel/driver/ps2.h>

enum  {
	DEV_UNKNOWN = 0,
	DEV_KEYBOARD,
	DEV_MOUSE,

	DEV_COUNT,
};

static const char *names[DEV_COUNT] = {
	"none",
	"kybd",
	"mous",
};
static const char *descs[DEV_COUNT] = {
	"PS/2 Device",
	"PS/2 Keyboard",
	"PS/2 Mouse",
};

static bool wait_int = true; /* ignore interrupts */
static device_t *dev_p0 = NULL, *dev_p1 = NULL; /* devices */
static bool rel = false; /* key was released */
static bool other = false; /* key was pressed */
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
	if (rel) key |= DEVICE_KEYCODE_RELEASE;

	rel = false;
	other = false;
	return key;
}

/* fill scancode sets with appropriate translations */
static void ps2_fill_scancode_sets(void) {

	scancodes2[0x01] = DEVICE_KEYCODE_F9;
	scancodes2[0x03] = DEVICE_KEYCODE_F5;
	scancodes2[0x04] = DEVICE_KEYCODE_F3;
	scancodes2[0x05] = DEVICE_KEYCODE_F1;
	scancodes2[0x06] = DEVICE_KEYCODE_F6;
	scancodes2[0x07] = DEVICE_KEYCODE_F12;
	scancodes2[0x09] = DEVICE_KEYCODE_F10;
	scancodes2[0x0a] = DEVICE_KEYCODE_F8;
	scancodes2[0x0b] = DEVICE_KEYCODE_F6;
	scancodes2[0x0c] = DEVICE_KEYCODE_F4;

	scancodes2[0x0d] = DEVICE_KEYCODE_TAB;
	scancodes2[0x0e] = DEVICE_KEYCODE_BACKTICK;
	scancodes2[0x11] = DEVICE_KEYCODE_LEFT_ALT;
	scancodes2[0x12] = DEVICE_KEYCODE_LEFT_SHIFT;
	scancodes2[0x14] = DEVICE_KEYCODE_LEFT_CONTROL;

	scancodes2[0x15] = DEVICE_KEYCODE_Q;
	scancodes2[0x16] = DEVICE_KEYCODE_1;
	scancodes2[0x1a] = DEVICE_KEYCODE_Z;
	scancodes2[0x1b] = DEVICE_KEYCODE_S;
	scancodes2[0x1c] = DEVICE_KEYCODE_A;
	scancodes2[0x1d] = DEVICE_KEYCODE_W;
	scancodes2[0x1e] = DEVICE_KEYCODE_2;
	scancodes2[0x21] = DEVICE_KEYCODE_C;
	scancodes2[0x22] = DEVICE_KEYCODE_X;
	scancodes2[0x23] = DEVICE_KEYCODE_D;
	scancodes2[0x24] = DEVICE_KEYCODE_E;
	scancodes2[0x25] = DEVICE_KEYCODE_4;
	scancodes2[0x26] = DEVICE_KEYCODE_3;
	scancodes2[0x29] = DEVICE_KEYCODE_SPACE;
	scancodes2[0x2a] = DEVICE_KEYCODE_V;
	scancodes2[0x2b] = DEVICE_KEYCODE_F;
	scancodes2[0x2c] = DEVICE_KEYCODE_T;
	scancodes2[0x2d] = DEVICE_KEYCODE_R;
	scancodes2[0x2e] = DEVICE_KEYCODE_5;
	scancodes2[0x31] = DEVICE_KEYCODE_N;
	scancodes2[0x32] = DEVICE_KEYCODE_B;
	scancodes2[0x33] = DEVICE_KEYCODE_H;
	scancodes2[0x34] = DEVICE_KEYCODE_G;
	scancodes2[0x35] = DEVICE_KEYCODE_Y;
	scancodes2[0x36] = DEVICE_KEYCODE_6;
	scancodes2[0x3a] = DEVICE_KEYCODE_M;
	scancodes2[0x3b] = DEVICE_KEYCODE_J;
	scancodes2[0x3c] = DEVICE_KEYCODE_U;
	scancodes2[0x3d] = DEVICE_KEYCODE_7;
	scancodes2[0x3e] = DEVICE_KEYCODE_8;
	scancodes2[0x41] = DEVICE_KEYCODE_COMMA;
	scancodes2[0x42] = DEVICE_KEYCODE_K;
	scancodes2[0x43] = DEVICE_KEYCODE_I;
	scancodes2[0x44] = DEVICE_KEYCODE_O;
	scancodes2[0x45] = DEVICE_KEYCODE_0;
	scancodes2[0x46] = DEVICE_KEYCODE_9;
	scancodes2[0x49] = DEVICE_KEYCODE_DOT;
	scancodes2[0x4a] = DEVICE_KEYCODE_SLASH;
	scancodes2[0x4b] = DEVICE_KEYCODE_L;
	scancodes2[0x4c] = DEVICE_KEYCODE_SEMICOLON;
	scancodes2[0x4d] = DEVICE_KEYCODE_P;
	scancodes2[0x4e] = DEVICE_KEYCODE_MINUS;
	scancodes2[0x52] = DEVICE_KEYCODE_QUOTE;
	scancodes2[0x54] = DEVICE_KEYCODE_LEFT_BRACKET;
	scancodes2[0x55] = DEVICE_KEYCODE_EQUALS;

	scancodes2[0x58] = DEVICE_KEYCODE_CAPS_LOCK;
	scancodes2[0x59] = DEVICE_KEYCODE_RIGHT_SHIFT;
	scancodes2[0x5a] = DEVICE_KEYCODE_RETURN;
	scancodes2[0x5b] = DEVICE_KEYCODE_RIGHT_BRACKET;
	scancodes2[0x5d] = DEVICE_KEYCODE_BACKSLASH;
	scancodes2[0x66] = DEVICE_KEYCODE_BACKSPACE;

	scancodes2[0x69] = DEVICE_KEYCODE_NUMPAD_1;
	scancodes2[0x6b] = DEVICE_KEYCODE_NUMPAD_4;
	scancodes2[0x6c] = DEVICE_KEYCODE_NUMPAD_7;
	scancodes2[0x70] = DEVICE_KEYCODE_NUMPAD_0;
	scancodes2[0x71] = DEVICE_KEYCODE_NUMPAD_DOT;
	scancodes2[0x72] = DEVICE_KEYCODE_NUMPAD_2;
	scancodes2[0x73] = DEVICE_KEYCODE_NUMPAD_5;
	scancodes2[0x74] = DEVICE_KEYCODE_NUMPAD_6;
	scancodes2[0x75] = DEVICE_KEYCODE_NUMPAD_8;
	scancodes2[0x76] = DEVICE_KEYCODE_ESCAPE;
	scancodes2[0x77] = DEVICE_KEYCODE_NUM_LOCK;
	scancodes2[0x78] = DEVICE_KEYCODE_F11;
	scancodes2[0x79] = DEVICE_KEYCODE_NUMPAD_PLUS;
	scancodes2[0x7a] = DEVICE_KEYCODE_NUMPAD_3;
	scancodes2[0x7b] = DEVICE_KEYCODE_NUMPAD_MINUS;
	scancodes2[0x7c] = DEVICE_KEYCODE_NUMPAD_ASTERISK;
	scancodes2[0x7d] = DEVICE_KEYCODE_NUMPAD_9;

	/* other scancodes */
	scancodes2_other[0x11] = DEVICE_KEYCODE_RIGHT_ALT;
	scancodes2_other[0x14] = DEVICE_KEYCODE_RIGHT_CONTROL;
	scancodes2_other[0x4a] = DEVICE_KEYCODE_NUMPAD_SLASH;
	scancodes2_other[0x5a] = DEVICE_KEYCODE_NUMPAD_ENTER;

	scancodes2_other[0x69] = DEVICE_KEYCODE_END;
	scancodes2_other[0x6b] = DEVICE_KEYCODE_LEFT;
	scancodes2_other[0x6c] = DEVICE_KEYCODE_HOME;
	scancodes2_other[0x70] = DEVICE_KEYCODE_INSERT;
	scancodes2_other[0x71] = DEVICE_KEYCODE_DELETE;
	scancodes2_other[0x72] = DEVICE_KEYCODE_DOWN;
	scancodes2_other[0x74] = DEVICE_KEYCODE_RIGHT;
	scancodes2_other[0x75] = DEVICE_KEYCODE_UP;
	scancodes2_other[0x7a] = DEVICE_KEYCODE_PAGE_DOWN;
	scancodes2_other[0x7d] = DEVICE_KEYCODE_PAGE_UP;
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

		kprintf(LOG_WARNING, "[ps/2 8042] self test failed");
		return;
	}

	/* test first port */
	ps2_wait_write();
	port_outb(PS2_PORT_CMD_STAT, PS2_CMD_TEST_P1);
	ps2_wait_read();
	if (port_inb(PS2_PORT_DATA) != 0) {

		kprintf(LOG_WARNING, "[ps/2 8042] port 1 test failed");
		return;
	}

	/* test second port */
	if (p2_pres) {

		ps2_wait_write();
		port_outb(PS2_PORT_CMD_STAT, PS2_CMD_TEST_P2);
		ps2_wait_read();
		if (port_inb(PS2_PORT_DATA) != 0) {

			kprintf(LOG_WARNING, "[ps/2 8042] port 2 test failed");
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

		kprintf(LOG_WARNING, "[ps/2 8042] failed to reset device on port 1");
		return;
	}

	if (p2_pres && ps2_reset_device(1) < 0) {

		kprintf(LOG_WARNING, "[ps/2 8042] failed to reset device on port 2");
		p2_pres = 0;
	}

	/* detect device types */
	dev_p0_type = ps2_get_device_type(0);
	if (p2_pres) dev_p1_type = ps2_get_device_type(1);

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
	dev_p0 = device_new(DEVICE_TYPE_CHAR, DEVICE_SUBTYPE_CHAR_PS2, names[dev_p0_type], descs[dev_p0_type], sizeof(device_char_t));
	kprintf(LOG_INFO, "[ps/2 8042] Detected first PS/2 device");

	device_char_t *inpdev_p0 = (device_char_t *)dev_p0;

	inpdev_p0->s_ibuf = 0;
	inpdev_p0->e_ibuf = 0;

	if (p2_pres) {
		
		dev_p1 = device_new(DEVICE_TYPE_CHAR, DEVICE_SUBTYPE_CHAR_PS2, names[dev_p1_type], descs[dev_p1_type], sizeof(device_char_t));
		kprintf(LOG_INFO, "[ps/2 8042] Detected second PS/2 device");
	}

	/* disable ignoring of interrupts */
	wait_int = false;
}

/* add devices */
extern void ps2_init_devfs(void) {

	if (dev_p0_type) {

		node_p0 = fs_node_new(NULL, FS_CHARDEVICE);
		devfs_add_node(dev_p0_type == DEV_KEYBOARD? "kbd": "mus", node_p0);
	}
	if (dev_p1_type) {

		node_p1 = fs_node_new(NULL, FS_CHARDEVICE);
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
	if (res != PS2_DEV_CMD_RES_ACK) return -1;

	ps2_wait_read();
	res = port_inb(PS2_PORT_DATA);
	if (res != PS2_DEV_CMD_RES_PASS) return -1;

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
	device_char_t *inpdev = (device_char_t *)dev;
	if (!inpdev) return;

	inpdev->ibuf[inpdev->e_ibuf] = key;
	inpdev->e_ibuf = (inpdev->e_ibuf + 1) % DEVICE_CHAR_BUFSZ;
}

/* mouse irq function */
static void ps2_irqmouse(idt_regs_t *regs, int d) {

	device_t *dev = d? dev_p1: dev_p0;

	/* get mouse data packet */
	uint8_t b = port_inb(PS2_PORT_DATA);

	uint8_t rx = port_inb(PS2_PORT_DATA);
	uint8_t ry = port_inb(PS2_PORT_DATA);

	int x = (uint8_t)rx;
	int y = (uint8_t)ry;

	if (b & PS2_MOUSE_FLAG_XS) x = -x;
	if (b & PS2_MOUSE_FLAG_YS) y = -y;
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
