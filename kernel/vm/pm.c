#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>
#include <string.h>
#include <sys/queue.h>

typedef struct region {
	paddr_t base;
	size_t pagecnt;

	TAILQ_ENTRY(region) entry;

	page_t pages[];
} region_t;

static TAILQ_HEAD(regionlist, region) regionlist;
static TAILQ_HEAD(pagelist, page) pagelist;
static spinlock_t page_lock;

paddr_t REAL_HHDM_START;

void pm_initialize()
{
	TAILQ_INIT(&regionlist);
	TAILQ_INIT(&pagelist);
	SPINLOCK_INIT(&page_lock);
}

void pm_add_region(paddr_t base, size_t length)
{
	region_t *region;
	size_t i, used;

	region = (region_t *)P2V(base).addr;
	region->pagecnt = length / PAGE_SIZE;
	region->base = base;
	TAILQ_INSERT_TAIL(&regionlist, region, entry);

	used = PAGE_ALIGN_UP(sizeof(region_t) +
			     sizeof(page_t) * length / PAGE_SIZE);
	if ((length - used) < PAGE_SIZE) {
		return;
	}

	pac_printf("pagecnt: %ld\n", region->pagecnt);

	for (i = 0; i < region->pagecnt; i++) {
		region->pages[i].usage = kPageUseFree;
		region->pages[i].pfn = (region->base.addr + i * PAGE_SIZE) >>
				       12;
		region->pages[i].region = region;
	}

	for (i = 0; i < used / PAGE_SIZE; i++) {
		region->pages[i].usage = kPageUseInternal;
	}

	for (; i < length / PAGE_SIZE; i++) {
#if PM_SORTED_INSERT == 0
		TAILQ_INSERT_TAIL(&pagelist, &region->pages[i], entry);
#else
		pm_free(&region->pages[i]);
#endif
	}

	pac_printf("added pm region 0x%lx (%ld pages)\n", base.addr,
		   length / PAGE_SIZE);
}

page_t *pm_allocate()
{
	page_t *page;
	spinlock_acquire(&page_lock);
	page = TAILQ_FIRST(&pagelist);
	TAILQ_REMOVE(&pagelist, page, entry);
	spinlock_release(&page_lock);
	return page;
}

page_t *pm_allocate_zeroed()
{
	page_t *page;
	page = pm_allocate();
	vaddr_t adr = PG2V(page);
	memset((void *)adr.addr, 0x0, PAGE_SIZE);
	return page;
}

void pm_free(page_t *page)
{
	spinlock_acquire(&page_lock);
#if PM_SORTED_INSERT == 0
	TAILQ_INSERT_HEAD(&pagelist, page, entry);
#else
	page_t *elm, *nearest_page = nullptr;
	TAILQ_FOREACH(elm, &pagelist, entry)
	{
		if (elm->pfn > page->pfn)
			break;
		nearest_page = elm;
	}

	if (nearest_page == nullptr) {
		TAILQ_INSERT_HEAD(&pagelist, page, entry);
	} else {
		TAILQ_INSERT_AFTER(&pagelist, nearest_page, page, entry);
	}
#endif
	spinlock_release(&page_lock);
}

page_t *pm_lookup(paddr_t paddr)
{
	region_t *region;
	TAILQ_FOREACH(region, &regionlist, entry)
	{
		if ((paddr.addr >= region->base.addr) &&
		    (paddr.addr <
		     region->base.addr + region->pagecnt * PAGE_SIZE)) {
			return &region->pages[(paddr.addr >> 12) -
					      region->pages[0].pfn];
		}
	}
	return nullptr;
}
