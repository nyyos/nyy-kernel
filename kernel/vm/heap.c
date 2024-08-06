#include <assert.h>
#include <lib/vmem.h>

#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>
#include <string.h>

static Vmem vmem_va;
static Vmem vmem_wired;
#define BUMP_ARENA_SIZE PAGE_SIZE * 5000
static char *g_bumparena;
static volatile size_t g_bump_pos;

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

	printk(DEBUG "allocate wired memory\n");

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

void kmem_init()
{
	vmem_bootstrap();
	vmem_init(&vmem_va, "kernel-va", (void *)MEM_KERNEL_START,
		  MEM_KERNEL_SIZE, PAGE_SIZE, nullptr, nullptr, nullptr, 0, 0);
	vm_kmap()->arena = &vmem_va;
	vmem_init(&vmem_wired, "kernel-wired", nullptr, 0, PAGE_SIZE,
		  &allocwired, &freewired, &vmem_va, 0, 0);
	printk(INFO "created kernel VA arena (%p-%p)\n",
	       (void *)MEM_KERNEL_START,
	       (void *)(MEM_KERNEL_START + MEM_KERNEL_SIZE));

	g_bump_pos = 0;
	g_bumparena = vmem_alloc(&vmem_wired, BUMP_ARENA_SIZE, VM_BESTFIT);
}

void *kmalloc(size_t size)
{
	size = ALIGN_UP(size, 16);
	size_t oldpos;
	size_t newpos;

	do {
		oldpos = __atomic_load_n(&g_bump_pos, __ATOMIC_ACQUIRE);
		newpos = oldpos + size;
		if (newpos > BUMP_ARENA_SIZE) {
			assert(!"go implement a slab alloc");
			return NULL;
		}
	} while (!__atomic_compare_exchange_n(&g_bump_pos, &oldpos, newpos, 1,
					      __ATOMIC_RELEASE,
					      __ATOMIC_RELAXED));

	return &g_bumparena[oldpos];
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
	// nop
}
