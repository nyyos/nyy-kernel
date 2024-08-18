#include <ndk/ndk.h>
#include <ndk/vm.h>
#include <ndk/internal/symbol.h>
#include <assert.h>

extern char addr_data_end[];
extern char addr_data_start[];

extern char addr_text_end[];
extern char addr_text_start[];

extern char addr_rodata_end[];
extern char addr_rodata_start[];

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
	while (base < end) {
		// XXX: check if gb pages are supported with cpuid
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

void remap_kernel(paddr_t paddr, vaddr_t vaddr)
{
	size_t offset = vaddr.addr - MEM_KERNEL_BASE;
	symbols_init(offset);

	vm_map_t *kmap = vm_kmap();

#define MAP_SECTION(SECTION, VMFLAGS)                                 \
	uintptr_t SECTION##_start =                                   \
		PAGE_ALIGN_DOWN((uint64_t)addr_##SECTION##_start);    \
	uintptr_t SECTION##_end =                                     \
		PAGE_ALIGN_UP((uint64_t)addr_##SECTION##_end);        \
	for (uintptr_t i = SECTION##_start; i < SECTION##_end;        \
	     i += PAGE_SIZE) {                                        \
		vm_port_map(kmap, PADDR(i - vaddr.addr + paddr.addr), \
			    VADDR(i), kVmWritethrough, (VMFLAGS));    \
	}

	MAP_SECTION(text, kVmKernelCode | kVmGlobal);
	MAP_SECTION(rodata, kVmRead | kVmGlobal);
	MAP_SECTION(data, kVmAll | kVmGlobal);

#undef MAP_SECTION

	printk(INFO "remapped kernel\n");
}
