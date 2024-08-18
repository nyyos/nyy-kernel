#include <ultra_protocol.h>
#include <ndk/addr.h>
#include <ndk/vm.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <ndk/internal/symbol.h>
#include <ndk/internal/boot.h>

extern void _port_init_boot_consoles();
extern void port_init_bsp(cpudata_t *bsp_data);
extern void port_data_common_init(cpudata_t *data);

extern cpudata_t bsp_data;

static struct ultra_memory_map_attribute *ultra_memmap;
static struct ultra_platform_info_attribute *ultra_pinfo;
static struct ultra_kernel_info_attribute *ultra_kinfo;

static void parse_ctx(struct ultra_boot_context *ctx)
{
	struct ultra_attribute_header *header = ctx->attributes;
	for (size_t i = 0; i < ctx->attribute_count;
	     i++, header = ULTRA_NEXT_ATTRIBUTE(header)) {
		switch (header->type) {
		case ULTRA_ATTRIBUTE_PLATFORM_INFO:
			ultra_pinfo =
				(struct ultra_platform_info_attribute *)header;
			break;
		case ULTRA_ATTRIBUTE_KERNEL_INFO:
			ultra_kinfo =
				(struct ultra_kernel_info_attribute *)header;
			break;
		case ULTRA_ATTRIBUTE_MEMORY_MAP:
			ultra_memmap =
				(struct ultra_memory_map_attribute *)header;
			break;
		}
	}
}

static void ultra_map_hhdm()
{
	struct ultra_memory_map_entry *entry;
	for (size_t i = 0;
	     i < ULTRA_MEMORY_MAP_ENTRY_COUNT(ultra_memmap->header); i++) {
		entry = &ultra_memmap->entries[i];
		int cache = kVmWriteback;

		if (entry->type == ULTRA_MEMORY_TYPE_KERNEL_BINARY)
			continue;

		remap_memmap_entry(PAGE_ALIGN_DOWN(entry->physical_address),
				   PAGE_ALIGN_UP(entry->size), cache);
	}
	printk(DEBUG "mapped hhdm\n");
}

void ultra_entry(struct ultra_boot_context *ctx, uint32_t magic)
{
	if (magic != ULTRA_MAGIC)
		__builtin_trap();
	REAL_HHDM_START = PADDR(MEM_HHDM_START);
	_printk_init();
	_port_init_boot_consoles();

	printk(INFO "Nyy//hyper " ARCHNAME " (Built on: " __DATE__ " " __TIME__
		    ")\n");

	port_init_bsp(&bsp_data);
	port_data_common_init(&bsp_data);
	cpudata()->bsp = true;

	parse_ctx(ctx);

	size_t entry_count = ULTRA_MEMORY_MAP_ENTRY_COUNT(ultra_memmap->header);
	for (size_t i = 0; i < entry_count; i++) {
		struct ultra_memory_map_entry *entry =
			&ultra_memmap->entries[i];
		if (entry->type == ULTRA_MEMORY_TYPE_FREE) {
			pm_add_region(PADDR(entry->physical_address),
				      entry->size);
		}
	}
	printk(INFO "initialized pm\n");

	vm_port_init_map(vm_kmap());
	remap_kernel(PADDR(ultra_kinfo->physical_base),
		     VADDR(ultra_kinfo->virtual_base));
	ultra_map_hhdm();
	vm_port_activate(vm_kmap());

	printk(WARN "TODO: implement hyper boot...\n");

	hcf();
}
