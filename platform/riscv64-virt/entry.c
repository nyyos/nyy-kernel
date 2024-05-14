#include "ndk/cpudata.h"
#include "ndk/irql.h"
#include <assert.h>
#include <limine.h>
#include <nanoprintf.h>
#include <ndk/ndk.h>
#include <ndk/vm.h>

#define LIMINE_REQ __attribute__((used, section(".requests")))

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

unsigned char *uart = (unsigned char *)0x10000000;
void pac_putc(int c, void *ctx)
{
	*uart = c;
}

irql_t irql_current()
{
	assert(!"todo");
}

void irql_set(irql_t irql)
{
	assert(!"todo");
}

void cpudata_port_setup(cpudata_port_t *cpudata)
{
	assert(!"todo");
}

void _start(void)
{
	REAL_HHDM_START = PADDR(hhdm_request.response->offset);

	pac_putc('h', NULL);
	hcf();

	pac_printf("Nyy/riscv64-virt (" __DATE__ " " __TIME__ ")\r\n");

	pm_initialize();

	struct limine_memmap_entry **entries = memmap_request.response->entries;
	for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
		struct limine_memmap_entry *entry = entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			pm_add_region(PADDR(entry->base), entry->length);
		}
	}
	pac_printf("initialized pm\n");

	pac_printf("reached kernel end\n");
	hcf();
}
