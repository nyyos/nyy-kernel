#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>
#include <string.h>
#include <sys/queue.h>
#include <nyyconf.h>

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
		pm_free(&region->pages[i]);
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
#if CONFIG_PM_SORT == 1
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
#else
	TAILQ_INSERT_HEAD(&pagelist, page, entry);
#endif
	spinlock_release(&page_lock);
}

#ifdef CONFIG_PM_ALLOC_NPAGES
page_t *pm_allocate_n(size_t n)
{
	page_t *page, *prev, *start;
	size_t count;

	count = 0;

	if (n == 0)
		return nullptr;
	if (n == 1)
		return pm_allocate();

	spinlock_acquire(&page_lock);

	TAILQ_FOREACH(page, &pagelist, entry)
	{
		prev = TAILQ_PREV(page, pagelist, entry);
		if (prev == nullptr) {
			continue;
		}
		if (prev->pfn + 1 == page->pfn) {
			if (count == 0)
				start = prev;
			count++;
			if (count > n) {
				for (size_t i = 0; i < n; i++) {
					TAILQ_REMOVE(&pagelist, &start[i],
						     entry);
				}
				break;
			}
		} else {
			count = 0;
			start = nullptr;
		}
	}

	spinlock_release(&page_lock);
	return start;
}

page_t *pm_allocate_n_zeroed(size_t n)
{
	page_t *pages;
	pages = pm_allocate_n(n);
	if (!pages)
		return nullptr;
	for (size_t i = 0; i < n; i++) {
		memset((void *)PG2V(&pages[i]).addr, 0x0, PAGE_SIZE);
	}
	return pages;
}

void pm_free_n(page_t *pages, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		pm_free(&pages[i]);
	}
}
#endif

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
