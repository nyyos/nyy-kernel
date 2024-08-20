#include <assert.h>
#include <string.h>

#include <lib/vmem.h>
#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>
#include <ndk/internal/slab_impl.h>
#include <ndk/kmem.h>

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

#if 0
	printk(DEBUG "allocated wired memory\n");
#endif

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
	printk(DEBUG "free wired memory\n");
}

void *vm_kalloc(size_t count, int flags)
{
	return vmem_alloc(&vmem_wired, count * PAGE_SIZE, flags);
}

void vm_kfree(void *addr, size_t count)
{
	vmem_free(&vmem_wired, addr, count * PAGE_SIZE);
}

static size_t kmalloc_sizes[] = { 8,   16,  32,	 64,  128,  160,
				  256, 512, 640, 768, 1024, 2048 };
static const char *kmalloc_names[] = {
	"kmalloc_8",   "kmalloc_16",  "kmalloc_32",   "kmalloc_64",
	"kmalloc_128", "kmalloc_160", "kmalloc_256",  "kmalloc_512",
	"kmalloc_640", "kmalloc_768", "kmalloc_1024", "kmalloc_2048"
};
static kmem_cache_t *kmalloc_caches[elementsof(kmalloc_sizes)];
#define KMALLOC_MAX 2048

void kmem_init()
{
	vmem_bootstrap();
	vmem_init(&vmem_va, "kernel-va", (void *)MEM_KERNEL_START,
		  MEM_KERNEL_SIZE, PAGE_SIZE, nullptr, nullptr, nullptr, 0, 0);
	vm_kmap()->arena = &vmem_va;
	vmem_init(&vmem_wired, "kernel-wired", nullptr, 0, PAGE_SIZE,
		  &allocwired, &freewired, &vmem_va, 0, 0);
	vmem_alloc(&vmem_va, 0x1000, 0);
	printk(INFO "created kernel VA arena (%p-%p)\n",
	       (void *)MEM_KERNEL_START,
	       (void *)(MEM_KERNEL_START + MEM_KERNEL_SIZE));

	kmem_slab_init();

	for (int i = 0; i < elementsof(kmalloc_sizes); i++) {
		kmalloc_caches[i] = kmem_cache_create(kmalloc_names[i],
						      kmalloc_sizes[i], 0, NULL,
						      NULL, NULL, 0);
	}
}

void *kmalloc(size_t size)
{
	for (int i = 0; i < elementsof(kmalloc_sizes); i++) {
		if (size <= kmalloc_sizes[i]) {
			return kmem_cache_alloc(kmalloc_caches[i], 0);
		}
	}
	return vm_kalloc(PAGE_ALIGN_UP(size), 0);
}

void *kcalloc(size_t count, size_t size)
{
	if (size == 0 || count == 0)
		return kmalloc(0);
	void *ptr = kmalloc(count * size);
	memset(ptr, 0x0, count * size);
	return ptr;
}

void kfree(void *ptr, size_t size)
{
	for (int i = 0; i < elementsof(kmalloc_sizes); i++) {
		if (size <= kmalloc_sizes[i]) {
			kmem_cache_free(kmalloc_caches[i], ptr);
			return;
		}
	}
	vm_kfree(ptr, PAGE_ALIGN_UP(size));
}
