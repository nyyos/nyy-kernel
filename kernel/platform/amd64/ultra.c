#include <string.h>
#include <ultra_protocol.h>
#include <ndk/addr.h>
#include <ndk/vm.h>
#include <ndk/kmem.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <ndk/internal/symbol.h>
#include <ndk/internal/boot.h>
#include <dkit/console.h>
#include <flanterm.h>
#include <backends/fb.h>
#include <assert.h>

extern void _port_init_boot_consoles();
extern void port_init_bsp(cpudata_t *bsp_data);
extern void port_data_common_init(cpudata_t *data);

extern cpudata_t bsp_data;

static struct ultra_memory_map_attribute *ultra_memmap;
static struct ultra_platform_info_attribute *ultra_pinfo;
static struct ultra_kernel_info_attribute *ultra_kinfo;
static struct ultra_module_info_attribute *ultra_symbol_map;
static struct ultra_framebuffer_attribute *ultra_framebuffer;

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
		case ULTRA_ATTRIBUTE_MODULE_INFO:
			struct ultra_module_info_attribute *modinfo =
				(struct ultra_module_info_attribute *)header;
			if (strncmp(modinfo->name, "kmap",
				    sizeof(modinfo->name)) == 0) {
				ultra_symbol_map = modinfo;
			}
			break;
		case ULTRA_ATTRIBUTE_MEMORY_MAP:
			ultra_memmap =
				(struct ultra_memory_map_attribute *)header;
			break;
		case ULTRA_ATTRIBUTE_FRAMEBUFFER_INFO:
			ultra_framebuffer =
				(struct ultra_framebuffer_attribute *)header;
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

typedef struct ultra_fb_console {
	console_t console;
	struct flanterm_context *ctx;
} ultra_fb_console_t;

void ultra_fb_write(console_t *console, const char *buf, size_t size)
{
	ultra_fb_console_t *fbcons = (ultra_fb_console_t *)console;
	flanterm_write(fbcons->ctx, buf, size);
}

static ultra_fb_console_t ultra_fb_console;

void ultra_fb_setup(struct ultra_framebuffer *fb)
{
	memset(&ultra_fb_console, 0x0, sizeof(ultra_fb_console_t));
	ultra_fb_console.console.name = "ultra-fb";
	ultra_fb_console.console.write = ultra_fb_write;

	uint8_t red_mask_size, red_mask_shift;
	uint8_t green_mask_size, green_mask_shift;
	uint8_t blue_mask_size, blue_mask_shift;

	switch (fb->format) {
	case ULTRA_FB_FORMAT_RGB888:
		blue_mask_shift = 0;
		blue_mask_size = 8;
		green_mask_shift = 8;
		green_mask_size = 8;
		red_mask_shift = 16;
		red_mask_size = 8;
		break;
	case ULTRA_FB_FORMAT_BGR888:
		red_mask_shift = 0;
		red_mask_size = 8;
		green_mask_shift = 8;
		green_mask_size = 8;
		blue_mask_shift = 16;
		blue_mask_size = 8;
		break;
	case ULTRA_FB_FORMAT_RGBX8888:
		blue_mask_shift = 8;
		blue_mask_size = 8;
		green_mask_shift = 16;
		green_mask_size = 8;
		red_mask_shift = 24;
		red_mask_size = 8;
		break;
	case ULTRA_FB_FORMAT_XRGB8888:
		blue_mask_shift = 0;
		blue_mask_size = 8;
		green_mask_shift = 8;
		green_mask_size = 8;
		red_mask_shift = 16;
		red_mask_size = 8;
		break;
	default:
		panic("ultra: invalid fb mode\n");
	}

	ultra_fb_console.ctx = flanterm_fb_init(
		NULL, NULL,
		(uint32_t *)(REAL_HHDM_START.addr + fb->physical_address),
		fb->width, fb->height, fb->pitch, red_mask_size, red_mask_shift,
		green_mask_size, green_mask_shift, blue_mask_size,
		blue_mask_shift, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		0, 0, 1, 0, 0, 0);
	assert(ultra_fb_console.ctx);

	console_add(&ultra_fb_console.console);
}

void ultra_entry(struct ultra_boot_context *ctx, uint32_t magic)
{
	if (magic != ULTRA_MAGIC)
		__builtin_trap();
	REAL_HHDM_START = PADDR(MEM_HHDM_START);

	port_init_bsp(&bsp_data);
	port_data_common_init(&bsp_data);
	cpudata()->bsp = true;

	_printk_init();
	_port_init_boot_consoles();

	parse_ctx(ctx);
	if (ultra_framebuffer != nullptr)
		ultra_fb_setup(&ultra_framebuffer->fb);

	printk(INFO "Nyy//hyper " ARCHNAME " (Built on: " __DATE__ " " __TIME__
		    ")\n");

	pm_init();
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

	struct ultra_framebuffer *fb = nullptr;
	if (ultra_framebuffer != nullptr) {
		fb = &ultra_framebuffer->fb;
		remap_memmap_entry(fb->physical_address,
				   PAGE_ALIGN_UP(fb->pitch * fb->height),
				   kVmWritecombine);
	}
	vm_port_activate(vm_kmap());

	kmem_init();

	if (ultra_symbol_map != nullptr) {
		symbols_parse_map((char *)ultra_symbol_map->address,
				  ultra_symbol_map->size);
	}

	printk(WARN "TODO: implement hyper boot...\n");

	hcf();
}
