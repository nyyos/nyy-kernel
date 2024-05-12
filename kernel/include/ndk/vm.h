#pragma once

#include <ndk/port.h>
#include <ndk/util.h>
#include <nyyconf.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

#define PAGE_ALIGN_UP(addr) ALIGN_UP(addr, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(addr) ALIGN_DOWN(addr, PAGE_SIZE)

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
#define PG2P(page) PADDR(((page)->pfn << 12))
#define PG2V(page) P2V(PG2P((page)))

extern paddr_t REAL_HHDM_START;

typedef struct vmstat {
	size_t total;
	size_t used;
} vmstat_t;

enum pageuse {
	kPageUseFree = 1,
	kPageUseInternal,
};

struct region;

typedef struct page {
	uint64_t pfn;
	short usage;
	TAILQ_ENTRY(page) entry;
	struct region *region;
} page_t;

/*
 * ================================
 * |   Physical Memory Manager    |
 * ================================
 */

// initialize basic PM structures
void pm_initialize();
// add a physical memory region into the pool
void pm_add_region(paddr_t base, size_t length);
// allocate/free functions
page_t *pm_allocate();
page_t *pm_allocate_zeroed();
page_t *pm_allocate_n(size_t n);
page_t *pm_allocate_n_zeroed(size_t n);
void pm_free_n(page_t *pages, size_t n);
void pm_free(page_t *page);
page_t *pm_lookup(paddr_t paddr);

void vmstat_dump();
