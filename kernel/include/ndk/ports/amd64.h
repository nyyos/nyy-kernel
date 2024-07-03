#pragma once

#include <ndk/addr.h>
#include <stdint.h>

#define PAGE_SIZE 0x1000

#define MEM_KERNEL_START 0xFFFFE00000000000L
#define MEM_KERNEL_SIZE TiB(1)

typedef struct cpudata_port {
	struct cpudata_port *self;

	uint64_t lapic_id;
} cpudata_port_t;

[[gnu::noreturn]] static inline void hcf()
{
	for (;;) {
		asm volatile("cli;hlt");
	}
}

typedef struct vm_port_map {
	paddr_t cr3;
} vm_port_map_t;

// also acts as "iframe"
typedef struct [[gnu::packed]] cpu_state {
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
} cpu_state_t;

#define STATE_SP(state) (state)->rsp
#define STATE_IP(state) (state)->rip
