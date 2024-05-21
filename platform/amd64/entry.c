#include <limine.h>
#include <nanoprintf.h>
#include <assert.h>
#include <stdint.h>

#include <ndk/ndk.h>
#include <nyyconf.h>
#include <ndk/vm.h>
#include <ndk/cpudata.h>
#include <ndk/port.h>
#include <ndk/kmem.h>
#include <lib/vmem.h>

#include "idt.h"
#include "gdt.h"
#include "asm.h"

#define LIMINE_REQ __attribute__((used, section(".requests")))
#define COM1 0x3f8

LIMINE_REQ
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used,
	       section(".requests_start_"
		       "marker"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
	used,
	section(".requests_end_marker"))) static volatile LIMINE_REQUESTS_END_MARKER;

LIMINE_REQ static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0
};

LIMINE_REQ static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 0
};

LIMINE_REQ static volatile struct limine_kernel_address_request
	address_request = {
		.id = LIMINE_KERNEL_ADDRESS_REQUEST,
		.revision = 0,
	};

static cpudata_t bsp_data;

static void serial_init()
{
	outb(COM1 + 1, 0x00); // Disable all interrupts
	outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	outb(COM1 + 1, 0x00); //                  (hi byte)
	outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
	outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

extern char addr_data_end[];
extern char addr_data_start[];

extern char addr_text_end[];
extern char addr_text_start[];

extern char addr_rodata_end[];
extern char addr_rodata_start[];

extern char addr_requests_end[];
extern char addr_requests_start[];

static void remap_kernel()
{
	paddr_t paddr = PADDR(address_request.response->physical_base);
	vaddr_t vaddr = VADDR(address_request.response->virtual_base);

	vm_map_t *kmap = vm_kmap();
	vm_port_init_map(kmap);

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

	MAP_SECTION(requests, kVmRead | kVmGlobal);
	MAP_SECTION(text, kVmKernelCode | kVmGlobal);
	MAP_SECTION(rodata, kVmRead | kVmGlobal);
	MAP_SECTION(data, kVmAll | kVmGlobal);

	struct limine_memmap_entry **entries = memmap_request.response->entries;
	for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
		struct limine_memmap_entry *entry = entries[i];
		if (entry->type == LIMINE_MEMMAP_RESERVED)
			continue;
		int cacheflags = kVmWritethrough;
		if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
			cacheflags = kVmUncached;
		}

		uintptr_t base = entry->base;
		uintptr_t end = base + entry->length;

		// map initial unaligned part until aligned to 2MB
		while (base < end) {
#ifdef CONFIG_HHDM_HUGEPAGES
			if ((base % MiB(2)) == 0) {
				break;
			}
#endif

			vm_port_map(kmap, PADDR(base),
				    VADDR(REAL_HHDM_START.addr + base),
				    cacheflags, kVmGlobal | kVmAll);
			base += PAGE_SIZE;
		}

#ifdef CONFIG_HHDM_HUGEPAGES
		// map using the largest possible pages
		while (base < end) {
			// XXX: check if gb pages are supported with cpuid
			if ((base % GiB(1)) == 0 && (base + GiB(1) <= end)) {
				vm_port_map(kmap, PADDR(base),
					    VADDR(REAL_HHDM_START.addr + base),
					    cacheflags,
					    kVmGlobal | kVmAll | kVmHuge1GB);
				base += GiB(1);
			} else if ((base % MiB(2)) == 0 &&
				   (base + MiB(2) <= end)) {
				vm_port_map(kmap, PADDR(base),
					    VADDR(REAL_HHDM_START.addr + base),
					    cacheflags,
					    kVmGlobal | kVmAll | kVmHuge2MB);
				base += MiB(2);
			} else {
				vm_port_map(kmap, PADDR(base),
					    VADDR(REAL_HHDM_START.addr + base),
					    cacheflags, kVmGlobal | kVmAll);
				base += PAGE_SIZE;
			}
		}
#endif

		assert(base <= end);
	}

	vm_port_activate(kmap);
}

void pac_putc(int c, void *ctx)
{
	while (!(inb(COM1 + 5) & 0x20))
		;
	outb(COM1, c);
}

void cpudata_port_setup(cpudata_port_t *cpudata)
{
	cpudata->self = cpudata;
	wrmsr(kMsrGsBase, (uint64_t)cpudata->self);
}

static void cpu_common_init(cpudata_t *cpudata)
{
	cpu_gdt_load();
	cpu_idt_load();

	cpudata_setup(cpudata);
}

cpudata_port_t *get_port_cpudata()
{
	cpudata_port_t *data = 0;
	asm volatile("mov %%gs:0x0, %0" : "=r"(data)::"memory");
	return data;
}

void _start(void)
{
	SPINLOCK_INIT(&g_pac_lock);
	REAL_HHDM_START = PADDR(hhdm_request.response->offset);

	serial_init();

	pac_printf("Nyy/amd64 (" __DATE__ " " __TIME__ ")\r\n");

	cpu_gdt_init();
	cpu_idt_init();

	cpu_common_init(&bsp_data);
	cpudata()->bsp = true;

	pm_initialize();

	struct limine_memmap_entry **entries = memmap_request.response->entries;
	for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
		struct limine_memmap_entry *entry = entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			pm_add_region(PADDR(entry->base), entry->length);
		}
	}
	pac_printf("initialized pm\r\n");

	remap_kernel();

	kmem_init();
	for (size_t i = 0; i < 32; i++) {
		void *p = kmalloc(i * 32 + 1);
		pac_printf("kmalloc:%p\n", p);
	}

	vmstat_dump();

	pac_printf("reached kernel end\r\n");
	hcf();
}
