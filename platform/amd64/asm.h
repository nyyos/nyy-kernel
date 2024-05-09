#pragma once

#include <stdint.h>

enum msr {
	kMsrLapicBase = 0x1B,
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

static inline void outb(uint16_t port, uint8_t val)
{
	asm volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
	return ret;
}
