#pragma once

#include "lib/vmem.h"
#include <ndk/port.h>
#include <ndk/util.h>
#include <ndk/addr.h>
#include <nyyconf.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

typedef struct vmstat {
	size_t total;
	size_t used;
} vmstat_t;

enum pageuse {
	// used nowhere
	kPageUseFree = 1,
	// used by pm structures
	kPageUseInternal,
	// used by pagetables or vm structures ...
	kPageUseVm,
	// wired in kernel
	kPageUseWired,
};

enum vmflags {
	kVmRead = 1,
	kVmWrite = 2,
	kVmExecute = 4,
	kVmUser = 8,
	kVmGlobal = 16,

	kVmHuge = 32,
	kVmHugeShift =
		26, // encode huge size in log2 in last 6 bits of 32 bit number

	kVmHuge2MB = (21 << kVmHugeShift) | kVmHuge,
	kVmHuge1GB = (30 << kVmHugeShift) | kVmHuge,

	kVmAll = kVmRead | kVmWrite,
	kVmUserAll = kVmAll | kVmUser,
	kVmKernelCode = kVmRead | kVmExecute,
	kVmUserCode = kVmRead | kVmExecute | kVmUser,
};

enum cachemode {
	kVmUncached,
	kVmWriteback,
	kVmWritethrough,
	kVmWritecombine,
};

struct region;

typedef struct page {
	uint64_t pfn;
	short usage;
} page_t;

typedef struct vm_map {
	Vmem *arena;
	vm_port_map_t portstate;
} vm_map_t;

/*
 * ================================
 * |   Physical Memory Manager    |
 * ================================
 */

// add a physical memory region into the pool
void pm_add_region(paddr_t base, size_t length);
// allocate/free functions
// all allocate functions are implemented using the pm_allocate_n function
// all free functions are implemented using the pm_free_n function
// @IRQL ANY
page_t *pm_allocate(short usage);
page_t *pm_allocate_zeroed(short usage);
page_t *pm_allocate_n(size_t n, short usage);
page_t *pm_allocate_n_zeroed(size_t n, short usage);
void pm_free_n(page_t *pages, size_t n);
void pm_free(page_t *page);
page_t *pm_lookup(paddr_t paddr);

/*
 * ========================================
 * |   Virtual Addressspace Management    |
 * ========================================
 */

void vmstat_dump();

vm_map_t *vm_map_create();
void vm_map_destroy(vm_map_t *map);
vm_map_t *vm_kmap();
void vm_setup_kmap();

void *vm_map_allocate(vm_map_t *map, size_t length);

void vm_port_init_map(vm_map_t *map);

void vm_port_unmap(vm_map_t *map, vaddr_t vaddr, uint64_t flags);
void vm_port_map(vm_map_t *map, paddr_t paddr, vaddr_t vaddr, uint64_t cache,
		 uint64_t flags);
paddr_t vm_port_translate_paddr(vm_map_t *map, vaddr_t addr);
page_t *vm_port_translate(vm_map_t *map, vaddr_t addr);
void vm_port_activate(vm_map_t *map);
