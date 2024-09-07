#pragma once

#include <ndk/addr.h>
#include <stdint.h>
#include <stddef.h>

#include "gdt.h"

#define PAGE_SIZE 0x1000

#define KSTACK_SIZE PAGE_SIZE * 4

#define MEM_HHDM_START 0xFFFF800000000000L
#define MEM_KERNEL_START 0xFFFFE00000000000L
#define MEM_KERNEL_SIZE TiB(1)
#define MEM_KERNEL_BASE 0xFFFFFFFF80000000L

typedef struct cpudata_port {
	struct cpudata_port *self;

	uint8_t lapic_id;
	uint64_t lapic_ticks_per_ms;
	bool lapic_calibrated;
} cpudata_port_t;

[[gnu::noreturn]] static inline void hcf()
{
	for (;;) {
		asm volatile("hlt");
	}
}

typedef struct vm_port_map {
	paddr_t cr3;
} vm_port_map_t;

typedef struct [[gnu::packed]] context {
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

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
} context_t;

static_assert(offsetof(context_t, rbx) == 8);
static_assert(offsetof(context_t, r12) == 80);
static_assert(offsetof(context_t, r13) == 88);
static_assert(offsetof(context_t, r14) == 96);
static_assert(offsetof(context_t, r15) == 104);
static_assert(offsetof(context_t, rbp) == 112);
static_assert(offsetof(context_t, rsp) == 144);

// XXX: FP state

#define STATE_SP(state) (state)->rsp
#define STATE_IP(state) (state)->rip
#define STATE_ARG1(state) (state)->rdi
#define STATE_ARG2(state) (state)->rsi
#define STATE_ARG3(state) (state)->rdx
#define STATE_ARG4(state) (state)->rcx
#define STATE_ARG5(state) (state)->r8
#define STATE_ARG6(state) (state)->r9

#define ARCH_HAS_SPIN_HINT
static inline void port_spin_hint()
{
	asm volatile("pause");
}

static inline void port_enable_ints()
{
	asm volatile("sti");
}

static inline void port_disable_ints()
{
	asm volatile("cli");
}

static inline int port_int_state()
{
	uint64_t flags;
	asm volatile("pushfq; pop %0" : "=rm"(flags)::"memory");
	return flags & (1 << 9);
}

static inline int port_set_ints(int state)
{
	uint64_t flags;
	asm volatile("pushfq; pop %0" : "=rm"(flags)::"memory");
	if (!state)
		asm volatile("cli");
	else
		asm volatile("sti");
	return flags & (1 << 9);
}

static inline void port_wait_nextint()
{
	asm volatile("hlt");
}

#define IRQ_VECTOR_MAX (256 - 32)
#define IRQL_VECTOR_COUNT_PER 16
#define IRQL_COUNT 16

enum IRQL_LEVELS {
	IRQL_PASSIVE = 0,
	IRQL_APC = 1,
	IRQL_DISPATCH = 2,
	IRQL_DEVICE = 3,
	IRQL_CLOCK = 13,
	IRQL_IPI = 14,
	IRQL_HIGH = 15,
};

#define IRQL_TO_VECTOR(irql) (((irql) << 4) - 32)
#define VECTOR_TO_IRQL(vec) ((vec) >> 4)

#define ARCH_HAS_MEMSET_IMPL
#define ARCH_HAS_MEMCPY_IMPL
