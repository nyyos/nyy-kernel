#include <backends/fb.h>
#include <string.h>
#include <flanterm.h>
#include <limine.h>
#include <nyyconf.h>
#include <limine-generic.h>
#include <ndk/addr.h>
#include <ndk/internal/boot.h>
#include <ndk/internal/symbol.h>
#include <ndk/vm.h>
#include <ndk/irq.h>
#include <ndk/kmem.h>
#include <ndk/ndk.h>
#include <ndk/time.h>
#include <ndk/cpudata.h>
#include <ndk/sched.h>
#include <ndk/dpc.h>
#include <nyyconf.h>
#include <DevKit/console.h>

#define LIMINE_REQ __attribute__((used, section(".requests")))

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

LIMINE_REQ static volatile struct limine_smp_request smp_request = {
	.id = LIMINE_SMP_REQUEST,
	.revision = 0
};

LIMINE_REQ volatile struct limine_framebuffer_request fb_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST,
	.revision = 0
};

LIMINE_REQ static volatile struct limine_module_request module_request = {
	.id = LIMINE_MODULE_REQUEST,
	.revision = 0
};

LIMINE_REQ static volatile struct limine_bootloader_info_request binfo_request = {
	.id = LIMINE_BOOTLOADER_INFO_REQUEST,
	.revision = 0,
};

LIMINE_REQ static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST,
	.revision = 0,
};

#ifdef CONFIG_SMP
#define MAX_CORES 16
extern void port_smp_entry(struct limine_smp_info *smp_info);

static struct smp_info ap_data[MAX_CORES];

void port_start_cores()
{
	printk(INFO "starting other cores\n");
	struct limine_smp_response *res = smp_request.response;
	for (size_t i = 0; i < res->cpu_count && i < MAX_CORES; i++) {
		struct limine_smp_info *cpu = res->cpus[i];
		if (cpu->lapic_id == 0)
			continue;
		struct smp_info *data = &ap_data[i];
		data->ready = false;
		cpu->extra_argument = (uint64_t)data;
		cpu->goto_address = port_smp_entry;
	}
	// synchronize all cores being up
	for (size_t i = 0; i < MAX_CORES && i < res->cpu_count; i++) {
		struct limine_smp_info *cpu = res->cpus[i];
		if (cpu->lapic_id == 0)
			continue;
		struct smp_info *info = &ap_data[i];
		while (__atomic_load_n(&info->ready, __ATOMIC_RELAXED) !=
		       true) {
			port_spin_hint();
		}
	}
	printk(INFO "all cores up\n");
}
#endif

typedef struct early_fb_console {
	console_t console;
	struct flanterm_context *ctx;
} early_fb_console_t;

void early_fb_write(console_t *console, const char *buf, size_t size)
{
	early_fb_console_t *fbcons = (early_fb_console_t *)console;
	flanterm_write(fbcons->ctx, buf, size);
}

static early_fb_console_t early_fb_console;
static void early_fb_init()
{
#ifdef CONFIG_EARLY_FRAMEBUFFER
	struct limine_framebuffer_response *res = fb_request.response;
	for (size_t i = 0; i < res->framebuffer_count; i++) {
		struct limine_framebuffer *fb = res->framebuffers[i];
		early_fb_console_t *cons = &early_fb_console;
		memset(cons, 0x0, sizeof(early_fb_console_t));
		cons->console.name = "limine-flanterm";
		cons->console.write = early_fb_write;
		cons->ctx = flanterm_fb_init(
			NULL, NULL, fb->address, fb->width, fb->height,
			fb->pitch, fb->red_mask_size, fb->red_mask_shift,
			fb->green_mask_size, fb->green_mask_shift,
			fb->blue_mask_size, fb->blue_mask_shift, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 1, 0, 0, 0);
		console_add((console_t *)cons);
		break;
	}
#endif
}

[[gnu::weak]] void limine_bootconsoles_init()
{
}

static void consume_modules()
{
	struct limine_module_response *res = module_request.response;
	for (size_t i = 0; i < res->module_count; i++) {
		struct limine_file *module = res->modules[i];
		if (strcmp("symbols", module->cmdline) == 0) {
			printk(DEBUG "got symbol map\n");
			symbols_parse_map(module->address, module->size);
		}
	}
}

extern void kmain();

void early_port_bootinfo()
{
	printk(INFO "Nyy//limine " ARCHNAME " (Built on: " __DATE__ " " __TIME__
		    ")\n");

	struct limine_bootloader_info_response *binfo = binfo_request.response;
	printk(INFO "booted using %s version %s-%ld\n", binfo->name,
	       binfo->version, binfo->revision);
}

void early_port_output()
{
	limine_bootconsoles_init();
	early_fb_init();
}

void early_port_add_pmem()
{
	struct limine_memmap_entry **entries = memmap_request.response->entries;
	for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
		struct limine_memmap_entry *entry = entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			pm_add_region(PADDR(entry->base), entry->length);
		}
	}
}

extern char addr_data_end[];
extern char addr_data_start[];
extern char addr_text_end[];
extern char addr_text_start[];
extern char addr_rodata_end[];
extern char addr_rodata_start[];

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
}

void early_port_map_memory()
{
	struct limine_kernel_address_response *address_res =
		address_request.response;
	struct limine_memmap_response *memmap_res = memmap_request.response;

	paddr_t paddr = PADDR(address_res->physical_base);
	vaddr_t vaddr = VADDR(address_res->virtual_base);

	remap_kernel(paddr, vaddr);

	printk(DEBUG "remapping hhdm\n");
	struct limine_memmap_entry **entries = memmap_res->entries;
	for (size_t i = 0; i < memmap_res->entry_count; i++) {
		struct limine_memmap_entry *entry = entries[i];
		uintptr_t base = PAGE_ALIGN_DOWN(entry->base);
		size_t length = PAGE_ALIGN_UP(entry->length);

		int cacheflags = kVmWriteback;
		if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
			cacheflags = kVmWritecombine;
		}

		remap_memmap_entry(base, length, cacheflags);
	}

	printk(DEBUG "remapped memory\n");
}

void early_port_post_kmem()
{
	consume_modules();
}

uintptr_t early_port_get_rsdp()
{
	struct limine_rsdp_response *res = rsdp_request.response;
	return (uintptr_t)res->address - REAL_HHDM_START.addr;
}

void limine_entry(void)
{
	REAL_HHDM_START = PADDR(hhdm_request.response->offset);
	kmain();
	__builtin_trap();
}
