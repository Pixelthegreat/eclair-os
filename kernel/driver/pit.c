#include <e.clair/types.h>
#include <e.clair/tty.h>
#include <e.clair/idt.h>
#include <e.clair/io/port.h>
#include <e.clair/driver/pit.h>

/* irq for pit */
static void pit_irq(idt_regs_t *regs) {
}

/* initialize pit */
extern void pit_init(void) {

	idt_set_irq_callback(0, pit_irq);

	pit_set_mode(PIT_COMMAND(PIT_CHANNEL0, PIT_ACCESS_LO, PIT_MODE_INT));
}

/* set operating mode */
extern void pit_set_mode(uint8_t mode) {

	port_outb(PIT_PORT_MODE, mode);
}

/* set value of channel */
extern void pit_set_channel(uint8_t ch, uint8_t val) {

	port_outb(PIT_PORT_CHANNEL0 + ch, val);
}
