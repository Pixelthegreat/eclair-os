#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/tty.h>
#include <kernel/boot.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/mm/heap.h>
#include <kernel/driver/pit.h>
#include <kernel/driver/rtc.h>
#include <kernel/driver/ps2.h>
#include <kernel/driver/ata.h>
#include <kernel/driver/vgacon.h>
#include <kernel/driver/fb.h>
#include <kernel/driver/fbcon.h>
#include <kernel/driver/uart.h>
#include <kernel/driver/pci.h>
#include <kernel/driver/device.h>

static boot_cmdline_t *cmdline; /* command line info */
static int devid = 1; /* current device id */

/* device classes */
devclass_t devclass_bus = DEVCLASS_INIT(sizeof(device_bus_t), "Buses");
devclass_t devclass_storage = DEVCLASS_INIT(sizeof(device_storage_t), "Storage");
devclass_t devclass_keyboard = DEVCLASS_INIT(sizeof(device_keyboard_t), "Keyboards");
devclass_t devclass_mouse = DEVCLASS_INIT(sizeof(device_mouse_t), "Mice");
devclass_t devclass_terminal = DEVCLASS_INIT(sizeof(device_t), "Terminals");
devclass_t devclass_video = DEVCLASS_INIT(sizeof(device_video_t), "Video Adapters");

/* initialize tty devices */
static void init_tty(void) {

	if (fb_addr) fbcon_init_tty();
	else vgacon_init_tty();

	uart_init(UART_COM1_BIT, UART_DEFAULT_BAUD_RATE);
	if (cmdline->uart_tty) uart_init_tty();
}

/* initialize all devices */
extern void device_init(void) {

	cmdline = boot_get_cmdline();

	init_tty();
	pit_init();
	ps2_init();
	ata_init();

	/* register pci drivers and initialize pci */
	driverinfo_t *drivers = DRIVERINFO_START;
	size_t count = DRIVERINFO_COUNT;

	for (size_t i = 0; i < count; i++) {
		if (drivers[i].type == DRIVERINFO_PCI)
			pci_register_driver((pci_driver_t *)drivers[i].data);
	}
	pci_init();
}

/* update devices */
extern void device_update(void) {

	if (fb_addr) fbcon_update();
}

/* create new device */
extern device_t *device_new(devclass_t *cls, const char *desc) {

	device_t *dev = (device_t *)kmalloc(cls->size);

	dev->cls = cls;
	if (desc) strncpy(dev->desc, desc, DEVICE_DESC_MAX_CHARS);
	else dev->desc[0] = 0;
	dev->impl = 0;
	dev->held = false;
	dev->clsnext = NULL;
	dev->busnext = NULL;
	dev->id = devid++;

	if (!cls->first) cls->first = dev;
	if (cls->last) cls->last->clsnext = dev;
	cls->last = dev;

	return dev;
}

/* print devices in class */
extern void device_print_class(devclass_t *cls) {

	kprintf(LOG_INFO, "[devices] %s:", cls->desc);

	device_t *cur = cls->first;
	while (cur) {

		kprintf(LOG_INFO, "[devices]  - %s", cur->desc);
		cur = cur->clsnext;
	}
}

/* debug */
extern void device_print_all(void) {

	device_print_class(&devclass_bus);
	device_print_class(&devclass_storage);
	device_print_class(&devclass_keyboard);
	device_print_class(&devclass_mouse);
	device_print_class(&devclass_terminal);
	device_print_class(&devclass_video);
}

/* create bus device */
extern device_t *device_bus_new(const char *desc) {

	device_t *dev = device_new(&devclass_bus, desc);
	device_bus_t *busdev = (device_bus_t *)dev;

	busdev->first = NULL;
	busdev->last = NULL;

	return dev;
}

/* create storage device */
extern device_t *device_storage_new(const char *desc) {

	device_t *dev = device_new(&devclass_storage, desc);
	device_storage_t *stdev = (device_storage_t *)dev;

	stdev->busy = false;
	stdev->read = NULL;
	stdev->write = NULL;

	return dev;
}

/* create keyboard device */
extern device_t *device_keyboard_new(const char *desc) {

	device_t *dev = device_new(&devclass_keyboard, desc);
	device_keyboard_t *kbdev = (device_keyboard_t *)dev;

	kbdev->kstart = 0;
	kbdev->kend = 0;

	return dev;
}

/* create mouse device */
extern device_t *device_mouse_new(const char *desc) {

	device_t *dev = device_new(&devclass_mouse, desc);
	device_mouse_t *msdev = (device_mouse_t *)dev;

	for (int i = 0; i < ECB_COUNT; i++)
		msdev->state[i] = false;
	msdev->evstart = 0;
	msdev->evend = 0;

	return dev;
}

/* create terminal device (stub) */
extern device_t *device_terminal_new(const char *desc) {

	return device_new(&devclass_terminal, desc);
}

/* create video device */
extern device_t *device_video_new(const char *desc) {

	return device_new(&devclass_video, desc);
}

/* read n blocks from storage device */
extern void device_storage_read(device_t *dev, uint32_t addr, size_t n, void *buf) {

	if (!dev || dev->cls != &devclass_storage) return;

	device_storage_t *stdev = (device_storage_t *)dev;
	if (stdev->read) stdev->read(dev, addr, n, buf);
}

/* write n blocks to storage device */
extern void device_storage_write(device_t *dev, uint32_t addr, size_t n, void *buf) {

	if (!dev || dev->cls != &devclass_storage) return;

	device_storage_t *stdev = (device_storage_t *)dev;
	if (stdev->write) stdev->write(dev, addr, n, buf);
}

/* add device to bus */
extern void device_bus_add(device_t *dev, device_t *child) {

	if (!dev || !child || dev->cls != &devclass_bus) return;
	device_bus_t *busdev = (device_bus_t *)dev;

	if (!busdev->first) busdev->first = child;
	if (busdev->last) busdev->last->busnext = child;
	busdev->last = child;
}

/* write key to ringbuffer */
extern void device_keyboard_putkey(device_t *dev, int key) {

	if (!dev || dev->cls != &devclass_keyboard) return;
	device_keyboard_t *kbdev = (device_keyboard_t *)dev;

	int end = (kbdev->kend + 1) % DEVICE_KEYBOARD_MAX_KEYS;
	if (end == kbdev->kstart) return;

	kbdev->keys[kbdev->kend] = key;
	kbdev->kend = end;
}

/* read key from ringbuffer */
extern int device_keyboard_getkey(device_t *dev) {

	if (!dev || dev->cls != &devclass_keyboard) return 0;
	device_keyboard_t *kbdev = (device_keyboard_t *)dev;

	if (kbdev->kstart == kbdev->kend) return 0;

	int key = kbdev->keys[kbdev->kstart];
	kbdev->kstart = (kbdev->kstart + 1) % DEVICE_KEYBOARD_MAX_KEYS;
	return key;
}

/* wait and read key from ringbuffer */
extern int device_keyboard_getkey_block(device_t *dev) {

	if (!dev || dev->cls != &devclass_keyboard) return 0;
	device_keyboard_t *kbdev = (device_keyboard_t *)dev;

	while (kbdev->kstart == kbdev->kend)
		asm volatile("hlt");

	int key = kbdev->keys[kbdev->kstart];
	kbdev->kstart = (kbdev->kstart + 1) % DEVICE_KEYBOARD_MAX_KEYS;
	return key;
}

/* write event to ringbuffer */
extern void device_mouse_putev(device_t *dev, device_mouse_event_t *ev) {

	if (!dev || dev->cls != &devclass_mouse) return;
	device_mouse_t *msdev = (device_mouse_t *)dev;

	int end = (msdev->evend + 1) % DEVICE_MOUSE_MAX_EVENTS;
	if (end == msdev->evstart) return;

	memcpy(&msdev->ev[msdev->evend], ev, sizeof(device_mouse_event_t));
	msdev->evend = end;
}

/* read event from ringbuffer */
extern device_mouse_event_t *device_mouse_getev(device_t *dev) {

	if (!dev || dev->cls != &devclass_mouse) return 0;
	device_mouse_t *msdev = (device_mouse_t *)dev;

	if (msdev->evstart == msdev->evend) return 0;

	device_mouse_event_t *ev = &msdev->ev[msdev->evstart];
	msdev->evstart = (msdev->evstart + 1) % DEVICE_KEYBOARD_MAX_KEYS;
	return ev;
}
