#pragma once

#include <stdint.h>

enum msr {
	kMsrLapicBase = 0x1B,
	kMsrPAT = 0x277,
	kMsrEfer = 0xC0000080,
	kMsrStar = 0xC0000081,
	kMsrLstar = 0xC0000082,
	kMsrFMask = 0xC0000083,
	kMsrFsBase = 0xC0000100,
	kMsrGsBase = 0xC0000101,
	kMsrKernelGsBase = 0xC0000102,
};

static inline void wrmsr(uint32_t index, uint64_t value)
{
	uint32_t low = value;
	uint32_t high = value >> 32;
	asm volatile("wrmsr" : : "c"(index), "a"(low), "d"(high) : "memory");
}

static inline uint64_t rdmsr(uint32_t index)
{
	uint32_t low, high;
	asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(index) : "memory");
	return ((uint64_t)high << 32) | (uint64_t)low;
}

static inline void outb(uint16_t port, uint8_t data)
{
	asm volatile("out %%al, %%dx" : : "a"(data), "d"(port));
}

static inline void outw(uint16_t port, uint16_t data)
{
	asm volatile("out %%ax, %%dx" : : "a"(data), "d"(port));
}

static inline void outl(uint16_t port, uint32_t data)
{
	asm volatile("out %%eax, %%dx" : : "a"(data), "d"(port));
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	asm volatile("in %%dx, %%al" : "=a"(ret) : "d"(port));
	return ret;
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t ret;
	asm volatile("in %%dx, %%ax" : "=a"(ret) : "d"(port));
	return ret;
}

static inline uint32_t inl(uint32_t port)
{
	uint32_t ret;
	asm volatile("in %%dx, %%eax" : "=a"(ret) : "d"(port));
	return ret;
}

#define FN_CR(REG)                                                 \
	static inline uint64_t read_cr##REG()                      \
	{                                                          \
		uint64_t data;                                     \
		asm volatile("mov %%cr" #REG ", %0" : "=r"(data)); \
		return data;                                       \
	}                                                          \
	static inline void write_cr##REG(uint64_t val)             \
	{                                                          \
		asm volatile("mov %0, %%cr" #REG ::"a"(val));      \
	}

FN_CR(0);
FN_CR(1);
FN_CR(2);
FN_CR(3);
FN_CR(4);
FN_CR(8);

static inline void cpuid(int code, uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
			 uint32_t *edx)
{
	asm volatile("cpuid"
		     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		     : "a"(code));
}
