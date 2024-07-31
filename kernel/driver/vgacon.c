#include <e.clair/types.h>
#include <e.clair/string.h>
#include <e.clair/tty.h>
#include <e.clair/io/port.h>
#include <e.clair/vfs/fs.h>
#include <e.clair/driver/vgacon.h>

static fs_node_t *dev = NULL;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_PORT_CONTROL 0x3d4
#define VGA_PORT_DATA 0x3d5
#define VGA_OFFSET_LOW 0x0f
#define VGA_OFFSET_HIGH 0x0e

static uint8_t *vidmem = (uint8_t *)0xC03FF000;
static uint32_t idx = 0;

static void vgacon_scroll(void) {

	if (idx < VGA_WIDTH * VGA_HEIGHT) return;

	/* copy */
	size_t lr = VGA_WIDTH * (VGA_HEIGHT-1); /* up to last row */
	for (uint32_t i = 0; i < lr * 2; i++)
		vidmem[i] = vidmem[i + VGA_WIDTH * 2];

	/* clear bottom row */
	memset(vidmem + lr * 2, 0, VGA_WIDTH * 2);
	idx = lr;
}

/* clear screen */
static void vgacon_clear(void) {

	memset(vidmem, 0, VGA_WIDTH * VGA_HEIGHT * 2);
}

/* print character */
static void vgacon_printc(char c) {

	if (c == '\n') idx += VGA_WIDTH - (idx % VGA_WIDTH);
	else {

		vidmem[idx * 2] = c;
		vidmem[idx * 2 + 1] = 0x7;
		idx++;
	}
	vgacon_scroll();

	/* update cursor position */
	port_outb(VGA_PORT_CONTROL, VGA_OFFSET_HIGH);
	port_outb(VGA_PORT_DATA, (idx >> 8) & 0xff);
	port_outb(VGA_PORT_CONTROL, VGA_OFFSET_LOW);
	port_outb(VGA_PORT_DATA, idx & 0xff);
}

/* read */
extern kssize_t vgacon_read(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf) {
}

/* write */
extern kssize_t vgacon_write(fs_node_t *_dev, uint32_t offset, size_t nbytes, uint8_t *buf) {

	for (size_t i = 0; i < nbytes; i++)
		vgacon_printc(((char *)buf)[i]);
}

/* base init */
extern void vgacon_init(void) {

	if (dev) return;

	dev = fs_node_new(NULL, FS_CHARDEVICE);
	dev->read = vgacon_read;
	dev->write = vgacon_write;

	vgacon_clear();
}

/* set kernel tty device */
extern void vgacon_set_tty(void) {

	tty_set_device(dev);
}

/* init and set tty device */
extern void vgacon_init_tty(void) {

	vgacon_init();
	vgacon_set_tty();
}
