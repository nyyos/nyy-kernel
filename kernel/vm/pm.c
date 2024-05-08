#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>
#include <sys/queue.h>

typedef struct region {
  paddr base;
  size_t pagecnt;

  TAILQ_ENTRY(region) entry;

  page_t pages[];
} region_t;

static TAILQ_HEAD(regionlist, region) regionlist;
static TAILQ_HEAD(pagelist, page) pagelist;

paddr REAL_HHDM_START;

void pm_initialize() {
  TAILQ_INIT(&regionlist);
  TAILQ_INIT(&pagelist);
}

void pm_add_region(paddr base, size_t length) {
  region_t *region;
  size_t i, used;

  region = (region_t *)P2V(base);
  region->pagecnt = length / PAGE_SIZE;
  region->base = base;
  TAILQ_INSERT_TAIL(&regionlist, region, entry);

  used = PAGE_ALIGN_UP(sizeof(region_t) + sizeof(page_t) * length / PAGE_SIZE);
  if ((length - used) < PAGE_SIZE) {
    return;
  }

  for (i = 0; i < region->pagecnt; i++) {
    region->pages[i].usage = kPageUseFree;
    region->pages[i].pfn = (region->base + i * PAGE_SIZE) >> 12;
  }

  for (i = 0; i < used / PAGE_SIZE; i++) {
    region->pages[i].usage = kPageUseInternal;
  }

  for (; i < length / PAGE_SIZE; i++) {
    TAILQ_INSERT_TAIL(&pagelist, &region->pages[i], entry);
  }

  pac_printf("added pm region 0x%lx (%ld pages)\n", base, length / PAGE_SIZE);
}

page_t *pm_allocate() {
  // LOCKME
  page_t *page = TAILQ_FIRST(&pagelist);
  TAILQ_REMOVE(&pagelist, page, entry);
  return page;
}

void pm_free(page_t *page) { TAILQ_INSERT_HEAD(&pagelist, page, entry); }
