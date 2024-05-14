#include <limine.h>
#include <nanoprintf.h>

#include <ndk/ndk.h>
#include <ndk/vm.h>
#include <ndk/cpudata.h>
#include <ndk/port.h>

#include "idt.h"
#include "gdt.h"
#include "asm.h"

#define LIMINE_REQ __attribute__((used, section(".requests")))
#define COM1 0x3f8

LIMINE_REQ
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used,
	       section(".requests_start_"
		       "marker"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
	used,
	section(".requests_end_marker"))) static volatile LIMINE_REQUESTS_END_MARKER;

LIMINE_REQ static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0
};

LIMINE_REQ static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 0
};

static cpudata_t bsp_data;

static void serial_init()
{
	outb(COM1 + 1, 0x00); // Disable all interrupts
	outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	outb(COM1 + 1, 0x00); //                  (hi byte)
	outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
	outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

void pac_putc(int c, void *ctx)
{
	while (!(inb(COM1 + 5) & 0x20))
		;
	outb(COM1, c);
}

void cpudata_port_setup(cpudata_port_t *cpudata)
{
	cpudata->self = cpudata;
	wrmsr(kMsrGsBase, (uint64_t)cpudata->self);
}

static void cpu_common_init(cpudata_t *cpudata)
{
	cpu_gdt_load();
	cpu_idt_load();

	cpudata_setup(cpudata);
}

cpudata_port_t *get_port_cpudata()
{
	cpudata_port_t *data = 0;
	asm volatile("mov %%gs:0x0, %0" : "=r"(data)::"memory");
	return data;
}

void _start(void)
{
	SPINLOCK_INIT(&g_pac_lock);
	REAL_HHDM_START = PADDR(hhdm_request.response->offset);

	serial_init();

	pac_printf("Nyy/amd64 (" __DATE__ " " __TIME__ ")\r\n");

	cpu_gdt_init();
	cpu_idt_init();

	cpu_common_init(&bsp_data);
	cpudata()->bsp = true;

	pm_initialize();

	struct limine_memmap_entry **entries = memmap_request.response->entries;
	for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
		struct limine_memmap_entry *entry = entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			pm_add_region(PADDR(entry->base), entry->length);
		}
	}
	pac_printf("initialized pm\r\n");

	vmstat_dump();

	pac_printf("reached kernel end\r\n");
	hcf();
}
