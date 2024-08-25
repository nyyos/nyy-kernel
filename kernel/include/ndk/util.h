#pragma once

#include <stddef.h>

#define ALIGN_UP(addr, align) (((addr) + align - 1) & ~(align - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define SWAP(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

#define DIV_ROUNDUP(a, b) (((a) + ((b) - 1)) / (b))

#define KiB(o) ((size_t)(1L << 10) * (o))
#define MiB(o) ((size_t)(1L << 20) * (o))
#define GiB(o) ((size_t)(1L << 30) * (o))
#define TiB(o) ((size_t)(1L << 40) * (o))

#define container_of(ptr, type, member)                            \
	({                                                         \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})

#define elementsof(arr) (sizeof((arr)) / sizeof((arr)[0]))

// beware: 0 is incorrectly considered a power of two by this
#define P2CHECK(v) (((v) & ((v) - 1)) == 0)

#define MAX(a, b)                       \
	({                              \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a > _b ? _a : _b;      \
	})

#define MIN(a, b)                       \
	({                              \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a < _b ? _a : _b;      \
	})
