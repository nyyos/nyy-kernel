#include <assert.h>
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

vmstat_t vmstat;
paddr_t REAL_HHDM_START;

void pm_initialize()
{
	TAILQ_INIT(&regionlist);
	TAILQ_INIT(&pagelist);
	SPINLOCK_INIT(&page_lock);
	memset(&vmstat, 0x0, sizeof(vmstat_t));
}

void pm_add_region(paddr_t base, size_t length)
{
	region_t *region;
	size_t i, used;

	region = (region_t *)P2V(base).addr;
	region->pagecnt = length / PAGE_SIZE;
	region->base = base;
	TAILQ_INSERT_TAIL(&regionlist, region, entry);

	vmstat.total += region->pagecnt;
	vmstat.used += region->pagecnt;

	used = PAGE_ALIGN_UP(sizeof(region_t) +
			     sizeof(page_t) * length / PAGE_SIZE);
	if ((length - used) < PAGE_SIZE) {
		return;
	}

	for (i = 0; i < region->pagecnt; i++) {
		region->pages[i].usage = kPageUseFree;
		region->pages[i].pfn = (region->base.addr + i * PAGE_SIZE) >>
				       12;
	}

	for (i = 0; i < used / PAGE_SIZE; i++) {
		region->pages[i].usage = kPageUseInternal;
	}

	pm_free_n(&region->pages[i], region->pagecnt - used / PAGE_SIZE);

	printk(DEBUG "added pm region 0x%lx (%ld pages)\n", base.addr,
		   length / PAGE_SIZE);
}

page_t *pm_allocate(short usage)
{
	return pm_allocate_n(1, usage);
}

page_t *pm_allocate_zeroed(short usage)
{
	return pm_allocate_n_zeroed(1, usage);
}

void pm_free(page_t *page)
{
	pm_free_n(page, 1);
}

page_t *pm_allocate_n(size_t n, short usage)
{
	page_t *elm, *prev, *start = nullptr;
	size_t count = 0;

	if (vmstat.total - vmstat.used < n)
		return nullptr;
	else if (n == 0)
		return nullptr;

	irql_t spinlock = spinlock_acquire(&page_lock, HIGH_LEVEL);

	if (n == 1) {
		elm = TAILQ_FIRST(&pagelist);
		TAILQ_REMOVE(&pagelist, elm, entry);
		vmstat.used += 1;
		goto cleanup;
	}

	TAILQ_FOREACH(elm, &pagelist, entry)
	{
		prev = TAILQ_PREV(elm, pagelist, entry);
		if (prev == nullptr) {
			count = 0;
			start = nullptr;
			continue;
		}
		if (prev->pfn + 1 == elm->pfn) {
			if (count == 0)
				start = prev;
			if (count == n) {
				for (size_t i = 0; i < n; i++) {
					TAILQ_REMOVE(&pagelist, &start[i],
						     entry);
				}
				break;
			}
			count++;
		} else {
			count = 0;
			start = nullptr;
		}
	}

	if (count < n)
		start = nullptr;

	if (start != nullptr) {
		vmstat.used += n;
		for (size_t i = 0; i < n; i++) {
#ifdef CONFIG_PM_CHECK
			assert(start[i].usage == kPageUseFree);
#endif
			start[i].usage = usage;
		}
	}

	elm = start;

cleanup:
	spinlock_release(&page_lock, spinlock);
	return elm;
}

page_t *pm_allocate_n_zeroed(size_t n, short usage)
{
	page_t *pages;
	pages = pm_allocate_n(n, usage);
	if (!pages)
		return nullptr;
	for (size_t i = 0; i < n; i++) {
		memset((void *)PG2V(&pages[i]).addr, 0x0, PAGE_SIZE);
	}
	return pages;
}

// skips search for page if:
// * empty list
// * smaller than queue head
// * bigger than queue tail
void pm_free_n(page_t *pages, size_t n)
{
	page_t *elm, *after, *last;
	irql_t spinlock = spinlock_acquire(&page_lock, HIGH_LEVEL);

	elm = TAILQ_FIRST(&pagelist);
	if (elm != nullptr && pages[0].pfn < elm->pfn) {
		for (size_t i = 0; i < n; i++) {
			// 'reverse' insert of pages
			TAILQ_INSERT_HEAD(&pagelist, &pages[n - i - 1], entry);
		}
		goto cleanup;
	}
	last = TAILQ_LAST(&pagelist, pagelist);
	if (elm == nullptr || last->pfn < pages[0].pfn) {
		for (size_t i = 0; i < n; i++) {
			TAILQ_INSERT_TAIL(&pagelist, &pages[i], entry);
		}
		goto cleanup;
	}

	after = nullptr;
	TAILQ_FOREACH(elm, &pagelist, entry)
	{
		if (elm->pfn < pages[0].pfn) {
			after = elm;
			break;
		}
	}

	for (size_t i = 0; i < n; i++) {
		TAILQ_INSERT_AFTER(&pagelist, after, &pages[i], entry);
		after = &pages[i];
	}

cleanup:
	for (size_t i = 0; i < n; i++) {
		pages[i].usage = kPageUseFree;
	}

#ifdef CONFIG_PM_CHECK
	page_t *tmp;
	TAILQ_FOREACH(tmp, &pagelist, entry)
	{
		page_t *prev = TAILQ_PREV(tmp, pagelist, entry);
		if (prev == nullptr)
			continue;
		assert(prev->pfn < tmp->pfn);
		assert(tmp->usage == kPageUseFree);
	}
#endif

	vmstat.used -= n;
	spinlock_release(&page_lock, spinlock);
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

void vmstat_dump()
{
	printk(
		"** VMSTAT **\n total: %ld\n used: %ld\n free: %ld\n** VMSTAT END **\n",
		vmstat.total, vmstat.used, vmstat.total - vmstat.used);
}
