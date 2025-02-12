#include <assert.h>
#include <stdint.h>
#include <ndk/vm.h>
#include <ndk/ndk.h>

#include "cpuid.h"

enum pte_masks {
	ptePresent = 0x1,
	pteWrite = 0x2,
	pteUser = 0x4,
	ptePwt = 0x8,
	ptePcd = 0x10,
	pteAccess = 0x20,
	pteModified = 0x40,
	ptePat = 0x80,
	ptePagesize = 0x80,
	pteGlobal = 0x100,
	pteAddress = 0x000FFFFFFFFFF000,
	pteNoExecute = 0x8000000000000000,
};

enum pteflags {
	pteFlagCreateNew = 1,
	pteFlag2MB = 2,
	pteFlag1GB = 4,
};

// always shift right by 12 to get non offset bits
// then every 9 bits is an index, 0x1ff to get those 9 bits
#define INDEX(addr, level) ((addr >> (12 + 9 * level)) & 0x1ff)

uint64_t *pte_walk(vm_map_t *map, vaddr_t vaddr, unsigned int flags)
{
	uintptr_t addr = vaddr.addr;
	// XXX: support for 5 lvl paging
	uint64_t *pml4 = (uint64_t *)P2V(map->portstate.cr3).addr;
	uint64_t *pml3, *pml2, *pml1;

	int pml4_index = (int)(INDEX(addr, 3));
	int pml3_index = (int)(INDEX(addr, 2));
	int pml2_index = (int)(INDEX(addr, 1));
	int pml1_index = (int)(INDEX(addr, 0));

	if ((pml4[pml4_index] & ptePresent) != 0) {
		pml3 = (uint64_t *)N2V((pml4[pml4_index] & pteAddress)).addr;
	} else {
		if ((flags & pteFlagCreateNew) == 0)
			return nullptr;
		page_t *page = pm_allocate_zeroed(kPageUseVm);
		assert(page);
		pml4[pml4_index] = PG2P(page).addr | ptePresent | pteUser |
				   pteWrite;
		pml3 = (uint64_t *)PG2V(page).addr;
	}

	if (flags & pteFlag1GB)
		return &pml3[pml3_index];

	if ((pml3[pml3_index] & ptePresent) != 0) {
		pml2 = (uint64_t *)N2V((pml3[pml3_index] & pteAddress)).addr;
	} else {
		if ((flags & pteFlagCreateNew) == 0)
			return nullptr;
		page_t *page = pm_allocate_zeroed(kPageUseVm);
		assert(page);
		pml3[pml3_index] = PG2P(page).addr | ptePresent | pteUser |
				   pteWrite;
		pml2 = (uint64_t *)PG2V(page).addr;
	}

	if (flags & pteFlag2MB)
		return &pml2[pml2_index];

	if ((pml2[pml2_index] & ptePresent) != 0) {
		pml1 = (uint64_t *)N2V((pml2[pml2_index] & pteAddress)).addr;
	} else {
		if ((flags & pteFlagCreateNew) == 0)
			return nullptr;
		page_t *page = pm_allocate_zeroed(kPageUseVm);
		assert(page);
		pml2[pml2_index] = PG2P(page).addr | ptePresent | pteUser |
				   pteWrite;
		pml1 = (uint64_t *)PG2V(page).addr;
	}

#if 0
	printk("idx: %d:%d:%d:%d\n", pml4_index, pml3_index, pml2_index,
		   pml1_index);
#endif
	return &pml1[pml1_index];
}

static inline void invlpg(vaddr_t vaddr)
{
	asm volatile("invlpg %0" ::"m"(vaddr.addr));
}

void vm_port_unmap(vm_map_t *map, vaddr_t vaddr, uint64_t flags)
{
	unsigned int walk_flags = 0;

	if (flags & kVmHuge) {
		switch (flags >> kVmHugeShift) {
		// log2 values
		case 12:
			break;
		case 21:
			walk_flags |= pteFlag2MB;
			break;
		case 30:
			assert(g_features.gbpages);
			walk_flags |= pteFlag1GB;
			break;
		default:
			assert(!"invalid huge page size");
		}
	}
	uint64_t *pte = pte_walk(map, vaddr, walk_flags);
	if (!pte)
		return;
	*pte &= ~(ptePresent | pteAddress);
}

void vm_port_map(vm_map_t *map, paddr_t paddr, vaddr_t vaddr, uint64_t cache,
		 uint64_t flags)
{
	uint64_t new_entry = 0;
	unsigned int walk_flags = pteFlagCreateNew;
	if (flags & kVmHuge) {
		switch (flags >> kVmHugeShift) {
		// log2 values
		case 21:
			assert((paddr.addr % MiB(2)) == 0);
			walk_flags |= pteFlag2MB;
			break;
		case 30:
			assert((paddr.addr % GiB(1)) == 0);
			if (!g_features.gbpages) {
				printk(DEBUG "emulating gb pages\n");
				for (int i = 0; i < 512; i++) {
					vm_port_map(
						map,
						PADDR(paddr.addr + MiB(2) * i),
						VADDR(vaddr.addr + MiB(2) * i),
						cache,
						(flags & ~(kVmHuge1GB)) |
							kVmHuge2MB);
				}
				return;
			}
			walk_flags |= pteFlag1GB;
			break;
		default:
			assert(!"invalid huge page size");
		}
		new_entry |= ptePagesize;
	}
	uint64_t *pte = pte_walk(map, vaddr, walk_flags);

	new_entry |= PAGE_ALIGN_DOWN(paddr.addr);
	if (flags & kVmUser)
		new_entry |= pteUser;
	if (flags & kVmWrite)
		new_entry |= pteWrite;
	if (flags & kVmRead)
		new_entry |= ptePresent;

	if (g_features.pge && (flags & kVmGlobal))
		new_entry |= pteGlobal;

	if (g_features.nx && !(flags & kVmExecute))
		new_entry |= pteNoExecute;

	switch (cache) {
		// PAT indexed like
		// [PAT|PCD|PWT]
	case kVmWritecombine:
		// PA5
		new_entry |= ptePat | ptePwt;
	case kVmWritethrough:
		// PA1
		new_entry |= ptePwt;
		break;
	case kVmWriteback:
		// PA0
		break;
	case kVmUncached:
		// PA3
		new_entry |= ptePcd | ptePwt;
		break;
	}

	*pte = new_entry;
	// XXX: shootdown
	invlpg(vaddr);
}

paddr_t vm_port_translate_paddr(vm_map_t *map, vaddr_t addr)
{
	uint64_t *pte = pte_walk(map, addr, 0);
	if (!pte)
		return PADDR(-1);
	return PADDR(*pte & pteAddress);
}

page_t *vm_port_translate(vm_map_t *map, vaddr_t addr)
{
	paddr_t paddr = vm_port_translate_paddr(map, addr);
	if (paddr.addr == -1)
		return NULL;
	return pm_lookup(paddr);
}

void vm_port_activate(vm_map_t *map)
{
	asm volatile("mov %0, %%cr3" ::"a"(map->portstate.cr3));
}

void vm_port_init_map(vm_map_t *map)
{
	page_t *pml4_page = pm_allocate_zeroed(kPageUseVm);
	assert(pml4_page);
	map->portstate.cr3 = PG2P(pml4_page);
}
