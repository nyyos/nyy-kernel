#pragma once

#define DUMP_STATE(state)                                                \
	do {                                                             \
		printk("rax: %016lx  rbx: %016lx\n", (state)->rax,       \
		       (state)->rbx);                                    \
		printk("rcx: %016lx  rdx: %016lx\n", (state)->rcx,       \
		       (state)->rdx);                                    \
		printk("rsi: %016lx  rdi: %016lx\n", (state)->rsi,       \
		       (state)->rdi);                                    \
		printk("r8:  %016lx  r9:  %016lx\n", (state)->r8,        \
		       (state)->r9);                                     \
		printk("r10: %016lx  r11: %016lx\n", (state)->r10,       \
		       (state)->r11);                                    \
		printk("r12: %016lx  r13: %016lx\n", (state)->r12,       \
		       (state)->r13);                                    \
		printk("r14: %016lx  r15: %016lx\n", (state)->r14,       \
		       (state)->r15);                                    \
		printk("rbp: %016lx  code: %016lx\n", (state)->rbp,      \
		       (state)->code);                                   \
		printk("rip: %016lx  cs:  %016lx\n", (state)->rip,       \
		       (state)->cs);                                     \
		printk("rflags: %016lx  rsp: %016lx\n", (state)->rflags, \
		       (state)->rsp);                                    \
		printk("ss:  %016lx\n", (state)->ss);                    \
		printk("\ncontrol registers:\n");                        \
		uint64_t cr2, cr3, cr4;                                  \
		asm volatile("mov %%cr2, %0" : "=r"(cr2));               \
		asm volatile("mov %%cr3, %0" : "=r"(cr3));               \
		asm volatile("mov %%cr4, %0" : "=r"(cr4));               \
		printk("CR2: 0x%016lx\n", cr2);                          \
		printk("CR3: 0x%016lx\n", cr3);                          \
		printk("CR4: 0x%016lx\n", cr4);                          \
	} while (0);
