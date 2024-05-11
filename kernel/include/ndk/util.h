#pragma once

#define ALIGN_UP(addr, align) (((addr) + align - 1) & ~(align - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align)-1))

#define DIV_ROUNDUP(a, b) (((a) + ((b)-1)) / (b))

#define KiB(o) (1 << 10) * o
#define MiB(o) (1 << 20) * o
#define GiB(o) (1 << 30) * o
#define TiB(o) (1 << 40) * o

#define container_of(ptr, type, member)                            \
	({                                                         \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})
