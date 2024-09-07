#include <string.h>
#include <ndk/vm.h>
#include <ndk/time.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include "hpet.h"

typedef struct hpet {
	uintptr_t addr;

	uint16_t minimum_clock_ticks;
	uint32_t period;
} hpet_t;

enum {
	HPET_REG_GENERAL_CAPABILITY = 0x0,
	HPET_REG_GENERAL_CONFIG = 0x10,
	HPET_REG_GENERAL_INT_STATUS = 0x20,
	HPET_REG_COUNTER_MAIN = 0xF0,
};

static hpet_t g_hpet;

static void hpet_write(const uint64_t reg, const uint64_t value)
{
	*(volatile uint64_t *)(g_hpet.addr + reg) = value;
}

static uint64_t hpet_read(const uint64_t reg)
{
	return *(volatile uint64_t *)(g_hpet.addr + reg);
}

uint64_t hpet_counter()
{
	return hpet_read(HPET_REG_COUNTER_MAIN);
}

void hpet_enable()
{
	hpet_write(HPET_REG_GENERAL_CONFIG,
		   hpet_read(HPET_REG_GENERAL_CONFIG) | (1 << 0));
}

void hpet_disable()
{
	hpet_write(HPET_REG_GENERAL_CONFIG,
		   hpet_read(HPET_REG_GENERAL_CONFIG) & ~(1 << 0));
}

uint64_t hpet_current_nanos()
{
	// XXX: account for past counter overflows
	return hpet_counter() * (g_hpet.period / NS2FEMTOS(1));
	return FEMTOS2NS(hpet_counter());
}

clocksource_t hpet_clocksource = {
	.name = "hpet",
	.priority = 100,
	.current_nanos = hpet_current_nanos,
};

int hpet_init()
{
	uacpi_table tbl;
	struct acpi_hpet *table;
	if (uacpi_table_find_by_signature("HPET", &tbl) != UACPI_STATUS_OK)
		return 1;
	table = tbl.ptr;

	memset(&g_hpet, 0x0, sizeof(hpet_t));
	g_hpet.addr = (uintptr_t)vm_map_allocate(vm_kmap(), PAGE_SIZE);
	vm_port_map(vm_kmap(), PADDR(PAGE_ALIGN_DOWN(table->address.address)),
		    VADDR(g_hpet.addr), kVmUncached, kVmRead | kVmWrite);
	g_hpet.minimum_clock_ticks = table->min_clock_tick;
	g_hpet.period = hpet_read(HPET_REG_GENERAL_CAPABILITY) >> 32;

	hpet_enable();
	register_clocksource(&hpet_clocksource);
	return 0;
}
