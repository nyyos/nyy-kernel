#include "asm.h"
#include "pic.h"

enum {
	PIC1_BASE = 0x20,
	PIC2_BASE = 0xA0,

	PIC1_CMD = PIC1_BASE,
	PIC1_DATA = (PIC1_BASE + 1),

	PIC2_CMD = PIC2_BASE,
	PIC2_DATA = (PIC2_BASE + 1),
};

void pic_disable()
{
	outb(PIC1_DATA, 0xFF);
	outb(PIC2_DATA, 0xFF);
}
