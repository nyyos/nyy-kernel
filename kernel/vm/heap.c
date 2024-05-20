#include <assert.h>
#include <lib/vmem.h>

#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>

static Vmem vmem_va;
static Vmem vmem_wired;

static void *allocwired(Vmem *vmem, size_t size, int vmflag)
{
	page_t *page;
	void *p = vmem_alloc(vmem, size, vmflag);

	for (uintptr_t i = (uintptr_t)p; i < (uintptr_t)p + size;
	     i += PAGE_SIZE) {
		page = pm_allocate(kPageUseWired);
		assert(page != nullptr);
		vm_port_map(vm_kmap(), PG2P(page), VADDR(i), kVmWritethrough,
			    kVmAll);
	}

	pac_printf("allocate wired memory\n");

	return p;
}

static void freewired(Vmem *vmem, void *ptr, size_t size)
{
	page_t *page;
	uintptr_t p = (uintptr_t)ptr;
	for (uintptr_t i = p; i < p + size; i += PAGE_SIZE) {
		page = vm_port_translate(vm_kmap(), VADDR(i));
		assert(page);
		vm_port_unmap(vm_kmap(), VADDR(i), 0);
		pm_free(page);
	}
	vmem_free(vmem, ptr, size);
	pac_printf("free wired memory\n");
}

void kmalloc_init()
{
	vmem_bootstrap();
	vmem_init(&vmem_va, "kernel-va", (void *)MEM_KERNEL_START,
		  MEM_KERNEL_SIZE, PAGE_SIZE, nullptr, nullptr, nullptr, 0, 0);
	vmem_init(&vmem_wired, "kernel-wired", nullptr, 0, PAGE_SIZE,
		  &allocwired, &freewired, &vmem_va, 0, 0);
	pac_printf("created kernel arena (%p-%p)\n", (void *)MEM_KERNEL_START,
		   (void *)(MEM_KERNEL_START + MEM_KERNEL_SIZE));
}
