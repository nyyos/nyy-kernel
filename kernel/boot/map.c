#include <ndk/ndk.h>
#include <ndk/vm.h>
#include <ndk/internal/symbol.h>
#include <assert.h>

void remap_memmap_entry(uintptr_t base, size_t length, int cache)
{
	vm_map_t *kmap = vm_kmap();
	uintptr_t end = base + length;
	//printk("base %#lx end %#lx\n", base, end);

	// map initial unaligned part until aligned to 2MB
	while (base < end) {
#ifdef AMD64
#ifdef CONFIG_HHDM_HUGEPAGES
		if ((base % MiB(2)) == 0) {
			break;
		}
#endif
#endif
		vm_port_map(kmap, PADDR(base),
			    VADDR(REAL_HHDM_START.addr + base), cache,
			    kVmGlobal | kVmAll);
		base += PAGE_SIZE;
	}

#ifdef AMD64
#ifdef CONFIG_HHDM_HUGEPAGES
	// map using the largest possible pages
	// 1gb pages are automatically emulated
	while (base < end) {
		if ((base % GiB(1)) == 0 && (base + GiB(1) <= end)) {
			vm_port_map(kmap, PADDR(base),
				    VADDR(REAL_HHDM_START.addr + base), cache,
				    kVmGlobal | kVmAll | kVmHuge1GB);
			base += GiB(1);
		} else if ((base % MiB(2)) == 0 && (base + MiB(2) <= end)) {
			vm_port_map(kmap, PADDR(base),
				    VADDR(REAL_HHDM_START.addr + base), cache,
				    kVmGlobal | kVmAll | kVmHuge2MB);
			base += MiB(2);
		} else {
			vm_port_map(kmap, PADDR(base),
				    VADDR(REAL_HHDM_START.addr + base), cache,
				    kVmGlobal | kVmAll);
			base += PAGE_SIZE;
		}
	}
#endif
#endif

	assert(base <= end);
}
