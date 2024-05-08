#include <limine.h>
#include <nanoprintf.h>
#include <ndk/ndk.h>
#include <ndk/vm.h>

#define LIMINE_REQ __attribute__((used, section(".requests")))

LIMINE_REQ
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used,
               section(".requests_start_"
                       "marker"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used,
    section(
        ".requests_end_marker"))) static volatile LIMINE_REQUESTS_END_MARKER;

LIMINE_REQ static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0};

LIMINE_REQ static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0};

static void hcf() {
  for (;;) {
    asm volatile("wfi");
  }
}

void pac_putc(int c, void *ctx) {}

void _start(void) {
  REAL_HHDM_START = hhdm_request.response->offset;

  pac_printf("Nyy/amd64 (" __DATE__ " " __TIME__ ")\r\n");

  pm_initialize();

  struct limine_memmap_entry **entries = memmap_request.response->entries;
  for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
    struct limine_memmap_entry *entry = entries[i];
    if (entry->type == LIMINE_MEMMAP_USABLE) {
      pm_add_region(entry->base, entry->length);
    }
  }
  pac_printf("initialized pm\n");

  pac_printf("reached kernel end\n");
  hcf();
}
