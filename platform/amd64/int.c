#include "gdt.h"
#include "idt.h"
#include "ndk/irql.h"
#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/ports/amd64/util.h>
#include <stdint.h>

#if defined(AMD64)
// 64 bit idt
typedef struct [[gnu::packed]] idt_entry {
	uint16_t isr_low;
	uint16_t kernel_cs;
	uint8_t ist;
	uint8_t attributes;
	uint16_t isr_mid;
	uint32_t isr_high;
	uint32_t rsv;
} idt_entry_t;

typedef struct [[gnu::packed]] idtr {
	uint16_t limit;
	uint64_t base;
} idtr_t;

void idt_make_entry(idt_entry_t *entry, uint64_t isr)
{
	entry->isr_low = (uint16_t)(isr);
	entry->isr_mid = (uint16_t)(isr >> 16);
	entry->isr_high = (uint32_t)(isr >> 32);
	entry->kernel_cs = kGdtKernelCode * 8;
	entry->rsv = 0;
	entry->ist = 0;
	entry->attributes = 0x8E;
}

#else
// 32 bit idt
typedef struct [[gnu::packed]] idt_entry {
	uint16_t isr_low;
	uint16_t kernel_cs;
	uint8_t rsv;
	uint8_t attributes;
	uint16_t isr_high;
} idt_entry_t;

typedef struct [[gnu::packed]] idtr {
	uint16_t limit;
	uint32_t base;
} idtr_t;
#endif

[[gnu::aligned(0x10)]] static idt_entry_t g_idt[256];

extern uint64_t itable[256];

void cpu_idt_init()
{
	for (int i = 0; i < 256; i++) {
		idt_entry_t *entry = &g_idt[i];
		idt_make_entry(entry, itable[i]);
	}
}

void cpu_idt_load()
{
	idtr_t idtr = { .limit = sizeof(idt_entry_t) * 256 - 1,
			.base = (uintptr_t)&g_idt[0] };
	asm volatile("lidt %0" ::"m"(idtr) : "memory");
}

void handle_pf(cpu_state_t *frame)
{
	int error_code = frame->code;
	pac_printf(LOG_DEBUG "Page Fault Error Code: 0x%x\n", error_code);
	pac_printf("Information: ");

	if (error_code & 0x1) {
		pac_printf("Present ");
	} else {
		pac_printf("Unpresent ");
	}

	if (error_code & 0x2) {
		pac_printf("Write ");
	} else {
		pac_printf("Read ");
	}

	if (error_code & 0x4) {
		pac_printf("User ");
	} else {
		pac_printf("Kernel ");
	}

	if (error_code & 0x8) {
		pac_printf("ReservedWrite ");
	}

	if (error_code & 0x10) {
		pac_printf("Instructionfetch ");
	}

	if (error_code & 0x20) {
		pac_printf("Protectionkey ");
	}

	if (error_code & 0x40) {
		pac_printf("Shadowstack ");
	}

	if (error_code & 0x80) {
		pac_printf("SGX-Violation ");
	}

	pac_printf("\n");
}

void handle_fault(cpu_state_t *frame, int number)
{
	spinlock_release(&g_pac_lock, PASSIVE_LEVEL);
	pac_printf("== FAULT ==\n");
	pac_printf("cpu state:\n");
	DUMP_STATE(frame);
	pac_printf("\n");
	if (number == 14) {
		handle_pf(frame);
	}
	hcf();
}

void handle_irq(cpu_state_t *frame, int number)
{
	pac_printf("got irq\n");
}

irql_t irql_current()
{
	uint64_t current;
	asm volatile("mov %%cr8, %0" : "=r"(current));
	return current;
}

void irql_set(irql_t irql)
{
	asm volatile("mov %0, %%cr8" ::"r"((uint64_t)irql));
}
