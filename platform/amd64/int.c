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

void idt_set_ist(idt_entry_t *entry, uint8_t ist);

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

	// nmi
	idt_set_ist(&g_idt[2], 1);
	// debug / breakpoint
	idt_set_ist(&g_idt[1], 2);
	idt_set_ist(&g_idt[3], 2);
	// double fault
	idt_set_ist(&g_idt[8], 3);
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
	printk(DEBUG "Page Fault Error Code: 0x%x\n", error_code);
	printk("Information: ");

	if (error_code & 0x1) {
		printk("Present ");
	} else {
		printk("Unpresent ");
	}

	if (error_code & 0x2) {
		printk("Write ");
	} else {
		printk("Read ");
	}

	if (error_code & 0x4) {
		printk("User ");
	} else {
		printk("Kernel ");
	}

	if (error_code & 0x8) {
		printk("ReservedWrite ");
	}

	if (error_code & 0x10) {
		printk("Instructionfetch ");
	}

	if (error_code & 0x20) {
		printk("Protectionkey ");
	}

	if (error_code & 0x40) {
		printk("Shadowstack ");
	}

	if (error_code & 0x80) {
		printk("SGX-Violation ");
	}

	printk("\n");
}

void handle_fault(cpu_state_t *frame, int number)
{
	printk(PANIC "== FAULT ==\n");
	printk(PANIC "cpu state:\n");
	DUMP_STATE(frame);
	printk(PANIC "\n");
	if (number == 14) {
		handle_pf(frame);
	}
	hcf();
}

void handle_irq(cpu_state_t *frame, int number)
{
	printk("got irq\n");
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
