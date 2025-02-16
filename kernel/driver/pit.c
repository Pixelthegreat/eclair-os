#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/idt.h>
#include <kernel/io/port.h>
#include <kernel/driver/pit.h>

static bool called = false;

/* temporary irq callback */
static void pit_irq(idt_regs_t *regs) {

	called = true;
}

/* initialize pit */
extern void pit_init(void) {

	idt_set_irq_callback(0, pit_irq);

	pit_set_mode(PIT_COMMAND(PIT_CHANNEL0, PIT_ACCESS_LO, PIT_MODE_INT));
	pit_set_channel(PIT_CHANNEL0, 0);
	while (!called);
}

/* delay */
extern void pit_delay(uint32_t div) {

	called = false;

	pit_set_mode(PIT_COMMAND(PIT_CHANNEL0, PIT_ACCESS_HILO, PIT_MODE_INT));
	pit_set_channel(PIT_CHANNEL0, (uint8_t)(div & 0xff));
	pit_set_channel(PIT_CHANNEL0, (uint8_t)((div >> 8) & 0xff));
	while (!called);
}

/* set operating mode */
extern void pit_set_mode(uint8_t mode) {

	port_outb(PIT_PORT_MODE, mode);
}

/* set value of channel */
extern void pit_set_channel(uint8_t ch, uint8_t val) {

	port_outb(PIT_PORT_CHANNEL0 + ch, val);
}

/* set timer callback */
extern void pit_set_callback(pit_callback_t cb) {

	idt_set_irq_callback(0, cb);
}
