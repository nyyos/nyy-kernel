#pragma once

#define DUMP_STATE(state)                                                    \
	do {                                                                 \
		pac_printf("rax: %016lx  rbx: %016lx\n", (state)->rax,       \
			   (state)->rbx);                                    \
		pac_printf("rcx: %016lx  rdx: %016lx\n", (state)->rcx,       \
			   (state)->rdx);                                    \
		pac_printf("rsi: %016lx  rdi: %016lx\n", (state)->rsi,       \
			   (state)->rdi);                                    \
		pac_printf("r8:  %016lx  r9:  %016lx\n", (state)->r8,        \
			   (state)->r9);                                     \
		pac_printf("r10: %016lx  r11: %016lx\n", (state)->r10,       \
			   (state)->r11);                                    \
		pac_printf("r12: %016lx  r13: %016lx\n", (state)->r12,       \
			   (state)->r13);                                    \
		pac_printf("r14: %016lx  r15: %016lx\n", (state)->r14,       \
			   (state)->r15);                                    \
		pac_printf("rbp: %016lx  code: %016lx\n", (state)->rbp,      \
			   (state)->code);                                   \
		pac_printf("rip: %016lx  cs:  %016lx\n", (state)->rip,       \
			   (state)->cs);                                     \
		pac_printf("rflags: %016lx  rsp: %016lx\n", (state)->rflags, \
			   (state)->rsp);                                    \
		pac_printf("ss:  %016lx\n", (state)->ss);                    \
		pac_printf("\ncontrol registers:\n");                        \
		uint64_t cr2, cr3, cr4;                                      \
		asm volatile("mov %%cr2, %0" : "=r"(cr2));                   \
		asm volatile("mov %%cr3, %0" : "=r"(cr3));                   \
		asm volatile("mov %%cr4, %0" : "=r"(cr4));                   \
		pac_printf("CR2: 0x%016lx\n", cr2);                          \
		pac_printf("CR3: 0x%016lx\n", cr3);                          \
		pac_printf("CR4: 0x%016lx\n", cr4);                          \
	} while (0);
