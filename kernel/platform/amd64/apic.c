#include "ndk/irq.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <ndk/addr.h>
#include <ndk/ndk.h>
#include <ndk/vm.h>
#include <ndk/cpudata.h>
#include <ndk/time.h>

#include "pic.h"
#include "asm.h"

enum {
	/* apic registers */
	APIC_REG_ID = 0x20,
	APIC_REG_VERSION = 0x30,
	APIC_REG_TPR = 0x80,
	APIC_REG_APR = 0x90,
	APIC_REG_PPR = 0xA0,
	APIC_REG_EOI = 0xB0,
	APIC_REG_RRD = 0xC0,
	APIC_REG_LOGICAL_DEST = 0xD0,
	APIC_REG_DEST_FORMAT = 0xE0,
	APIC_REG_SPURIOUS = 0xF0,
	APIC_REG_ISR = 0x100,
	APIC_REG_TMR = 0x180,
	APIC_REG_IRR = 0x200,
	APIC_REG_STATUS = 0x280,
	APIC_REG_CMCI = 0x2F0,
	APIC_REG_ICR0 = 0x300,
	APIC_REG_ICR1 = 0x310,
	APIC_REG_LVT_TIMER = 0x320,
	APIC_REG_LVT_THERMAL = 0x330,
	APIC_REG_LVT_PERFMON = 0x340,
	APIC_REG_LVT_LINT0 = 0x350,
	APIC_REG_LVT_LINT1 = 0x360,
	APIC_REG_LVT_ERROR = 0x370,
	/* apic timer registers */
	APIC_REG_TIMER_INITIAL = 0x380,
	APIC_REG_TIMER_CURRENT = 0x390,
	APIC_REG_TIMER_DIVIDE = 0x3E0,
};

typedef struct apic {
	vaddr_t virt_base;
	irq_t *timer_irq;
} apic_t;

static apic_t g_apic;

static paddr_t apic_phys_base()
{
	return PADDR(rdmsr(kMsrLapicBase) & 0xffffffffff000);
}

static void apic_write(uint16_t offset, uint32_t value)
{
	assert((offset & 15) == 0);
	*(volatile uint32_t *)(g_apic.virt_base.addr + offset) = value;
}

static uint32_t apic_read(uint16_t offset)
{
	assert((offset & 15) == 0);
	return *(volatile uint32_t *)(g_apic.virt_base.addr + offset);
}

uint8_t apic_id()
{
	return apic_read(APIC_REG_ID) >> 24;
}

uint8_t apic_version()
{
	return apic_read(APIC_REG_VERSION) & 0xff;
}

void apic_eoi()
{
	apic_write(APIC_REG_EOI, 0);
}

static void apic_timer_calibrate()
{
	int wait_ms = 50;
	/*
	0000 - Divide by 2
	0001 - Divide by 4
	0010 - Divide by 8
	0011 - Divide by 16
	1000 - Divide by 32
	1001 - Divide by 64
	1010 - Divide by 128
	1011 - Divide by 1
	*/
	apic_write(APIC_REG_TIMER_DIVIDE, 0b0011);
	uint32_t start_val = UINT32_MAX;
	apic_write(APIC_REG_TIMER_INITIAL, start_val);
	time_delay(MS2NS(wait_ms));
	uint32_t elapsed = start_val - apic_read(APIC_REG_TIMER_CURRENT);
	apic_write(APIC_REG_TIMER_INITIAL, 0);

	cpudata()->port_data.lapic_ticks_per_ms = elapsed / wait_ms;
	printk(DEBUG "apic timer: %ld ticks/ms\n",
	       cpudata()->port_data.lapic_ticks_per_ms);
	cpudata()->port_data.lapic_calibrated = true;
}

void apic_send_ipi(uint32_t lapic_id, vector_t vector)
{
	apic_write(APIC_REG_ICR1, lapic_id << 24);
	apic_write(APIC_REG_ICR0, vector);
}

void apic_arm(uint64_t deadline)
{
	// i stole this from managram
	if (deadline == 0) {
		apic_write(APIC_REG_TIMER_INITIAL, 0);
		return;
	}
	uint64_t ticks;
	uint64_t now = clocksource()->current_nanos();
	if (deadline < now) {
		ticks = 1;
	} else {
		bool of = __builtin_mul_overflow(
			(deadline - now),
			cpudata()->port_data.lapic_ticks_per_ms, &ticks);
		assert(!of);
		ticks /= 1000000;
		if (!ticks) {
			ticks = 1;
		}
	}

	apic_write(APIC_REG_TIMER_INITIAL, ticks);
}

int timer_handler(irq_t *obj, context_t *frame, void *private)
{
	(void)obj;
	(void)frame;
	(void)private;
	if (clocksource()->current_nanos() >= cpudata()->next_deadline) {
		dpc_enqueue(&cpudata()->timer_update, NULL, NULL);
	}
	return kIrqAck;
}

void apic_enable()
{
	// mask everything
	apic_write(APIC_REG_LVT_TIMER, (1 << 16));
	apic_write(APIC_REG_LVT_ERROR, (1 << 16));
	apic_write(APIC_REG_LVT_LINT0, (1 << 16));
	apic_write(APIC_REG_LVT_LINT1, (1 << 16));
	apic_write(APIC_REG_LVT_PERFMON, (1 << 16));
	apic_write(APIC_REG_LVT_THERMAL, (1 << 16));
	apic_eoi();
	// send spurious interrupts to 0xFF, enable APIC
	apic_write(APIC_REG_SPURIOUS, (1 << 8) | 0xFF);

	cpudata()->port_data.lapic_id = apic_id();

	apic_timer_calibrate();
	time_engine_init(&cpudata()->timer_engine, clocksource(), apic_arm);
	apic_write(APIC_REG_LVT_TIMER, g_apic.timer_irq->vector->vector + 32);

	apic_eoi();
}

void apic_init()
{
	memset(&g_apic, 0x0, sizeof(apic_t));
	paddr_t phys_base = apic_phys_base();
	g_apic.virt_base = VADDR(vm_map_allocate(vm_kmap(), PAGE_SIZE));
	vm_port_map(vm_kmap(), phys_base, g_apic.virt_base, kVmUncached,
		    kVmRead | kVmWrite);

	pic_disable();

	g_apic.timer_irq = irq_allocate_obj();
	irq_initialize_obj_irql(g_apic.timer_irq, IRQL_CLOCK, IRQ_FORCE);
	g_apic.timer_irq->handler = timer_handler;
}
