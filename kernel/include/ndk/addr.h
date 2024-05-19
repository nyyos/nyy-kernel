#pragma once

#include <stdint.h>

#define PAGE_ALIGN_UP(addr) ALIGN_UP(addr, PAGE_SIZE);
#define PAGE_ALIGN_DOWN(addr) ALIGN_DOWN(addr, PAGE_SIZE);

typedef struct paddr {
	uintptr_t addr;
} paddr_t;

typedef struct vaddr {
	uintptr_t addr;
} vaddr_t;

#define PADDR(fromaddr)                       \
	(paddr_t)                             \
	{                                     \
		.addr = (uintptr_t)(fromaddr) \
	}
#define VADDR(fromaddr)                       \
	(vaddr_t)                             \
	{                                     \
		.addr = (uintptr_t)(fromaddr) \
	}

#ifndef P2V
#define P2V(paddr) VADDR(((paddr).addr + REAL_HHDM_START.addr))
#endif
#define N2V(n) (P2V(PADDR(n)))
#define PG2P(page) PADDR(((page)->pfn << 12))
#define PG2V(page) P2V(PG2P((page)))

extern paddr_t REAL_HHDM_START;
