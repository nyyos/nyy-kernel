#include "gdt.h"
#include "idt.h"
#include "ndk/irql.h"
#include <ndk/ndk.h>
#include <ndk/port.h>
#include <stdint.h>

#if defined(AMD64)
typedef struct [[gnu::packed]] iframe {
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t rbp;

	uint64_t code;

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
} iframe_t;

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

void handle_pf(iframe_t *frame)
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

void handle_fault(iframe_t *frame, int number)
{
	spinlock_release(&g_pac_lock, PASSIVE_LEVEL);
	pac_printf("== FAULT ==\n");
	pac_printf("cpu state:\n");
	pac_printf("rax: %016lx  rbx: %016lx\n", frame->rax, frame->rbx);
	pac_printf("rcx: %016lx  rdx: %016lx\n", frame->rcx, frame->rdx);
	pac_printf("rsi: %016lx  rdi: %016lx\n", frame->rsi, frame->rdi);
	pac_printf("r8:  %016lx  r9:  %016lx\n", frame->r8, frame->r9);
	pac_printf("r10: %016lx  r11: %016lx\n", frame->r10, frame->r11);
	pac_printf("r12: %016lx  r13: %016lx\n", frame->r12, frame->r13);
	pac_printf("r14: %016lx  r15: %016lx\n", frame->r14, frame->r15);
	pac_printf("rbp: %016lx  code: %016lx\n", frame->rbp, frame->code);
	pac_printf("rip: %016lx  cs:  %016lx\n", frame->rip, frame->cs);
	pac_printf("rflags: %016lx  rsp: %016lx\n", frame->rflags, frame->rsp);
	pac_printf("ss:  %016lx\n", frame->ss);
	pac_printf("\ncontrol registers:\n");
	uint64_t cr2, cr3, cr4;
	asm volatile("mov %%cr2, %0" : "=r"(cr2));
	asm volatile("mov %%cr3, %0" : "=r"(cr3));
	asm volatile("mov %%cr4, %0" : "=r"(cr4));
	pac_printf("CR2: 0x%016lx\n", cr2);
	pac_printf("CR3: 0x%016lx\n", cr3);
	pac_printf("CR4: 0x%016lx\n", cr4);

	pac_printf("\n");
	if (number == 14) {
		handle_pf(frame);
	}
	hcf();
}

void handle_irq(iframe_t *frame, int number)
{
	pac_printf("got irq\n");
}

void handle_timer_irq(void *frame)
{
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
