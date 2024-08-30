#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>
#include <string.h>
#include <sys/queue.h>
#include <assert.h>
#include <nyyconf.h>

typedef struct region {
	TAILQ_ENTRY(region) entry;
	paddr_t base;
	size_t pagecnt;
	page_t pages[];
} region_t;

static TAILQ_HEAD(, region) regionlist = TAILQ_HEAD_INITIALIZER(regionlist);

// order 0 = PAGE_SIZE
#define BUDDY_ORDERS 10
static TAILQ_HEAD(, page) buddy_freelist[BUDDY_ORDERS];
static spinlock_t buddy_lock;

vmstat_t vmstat = { 0, 0 };
paddr_t REAL_HHDM_START;

void pm_init()
{
	for (int i = 0; i < BUDDY_ORDERS; i++) {
		TAILQ_INIT(&buddy_freelist[i]);
	}
}

void pm_add_region(paddr_t base, size_t length)
{
	region_t *region;
	size_t i, used;

	used = PAGE_ALIGN_UP(sizeof(region_t) +
			     sizeof(page_t) * length / PAGE_SIZE);
	if ((length - used) < PAGE_SIZE * 2) {
		return;
	}

	irql_t irql = spinlock_acquire(&buddy_lock, IRQL_HIGH);
	region = (region_t *)P2V(base).addr;
	region->pagecnt = length / PAGE_SIZE;
	region->base = base;
	TAILQ_INSERT_TAIL(&regionlist, region, entry);
	vmstat.total += region->pagecnt;

	for (i = 0; i < region->pagecnt; i++) {
		region->pages[i].usage = kPageUseFree;
		region->pages[i].pfn = (region->base.addr + i * PAGE_SIZE) >>
				       12;
		region->pages[i].order = 0;
	}

	for (i = 0; i < used / PAGE_SIZE; i++) {
		region->pages[i].usage = kPageUseInternal;
	}

	for (; i < region->pagecnt; i++) {
		page_t *page = &region->pages[i];
		TAILQ_INSERT_TAIL(&buddy_freelist[page->order], page,
				  queue_entry);
	}

	vmstat.used += used / PAGE_SIZE;

	spinlock_release(&buddy_lock, irql);

	printk(DEBUG "added pm region 0x%lx (%ld/%ld pages usable)\n",
	       base.addr, (length - used) / PAGE_SIZE, length / PAGE_SIZE);
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

static inline page_t *pm_lookup_with_region(paddr_t paddr, region_t *region)
{
	return &region->pages[(paddr.addr - region->base.addr) / PAGE_SIZE];
}

page_t *pm_allocate_n(size_t n, short usage)
{
	assert(n == 1);
	irql_t irql = spinlock_acquire(&buddy_lock, IRQL_HIGH);
	page_t *pg = TAILQ_FIRST(&buddy_freelist[0]);
	pg->usage = usage;
	TAILQ_REMOVE(&buddy_freelist[0], pg, queue_entry);
	__atomic_fetch_add(&vmstat.used, n, __ATOMIC_RELAXED);
	spinlock_release(&buddy_lock, irql);
	return pg;
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

static region_t *pm_lookup_region(paddr_t paddr)
{
	region_t *region;
	TAILQ_FOREACH(region, &regionlist, entry)
	{
		if ((paddr.addr >= region->base.addr) &&
		    (paddr.addr <

		     region->base.addr + region->pagecnt * PAGE_SIZE)) {
			return region;
		}
	}
	return nullptr;
}

page_t *pm_lookup(paddr_t paddr)
{
	region_t *region = pm_lookup_region(paddr);
	if (region == nullptr)
		return nullptr;
	return pm_lookup_with_region(paddr, region);
}

void pm_free_n(page_t *pages, size_t n)
{
	assert(n == 1);
	irql_t irql = spinlock_acquire(&buddy_lock, IRQL_HIGH);
	for (size_t i = 0; i < n; i++) {
		pages[i].usage = kPageUseFree;
	}
	TAILQ_INSERT_TAIL(&buddy_freelist[0], pages, queue_entry);
	__atomic_fetch_sub(&vmstat.used, n, __ATOMIC_RELAXED);
	spinlock_release(&buddy_lock, irql);
}

void vmstat_dump()
{
	size_t used = __atomic_load_n(&vmstat.used, __ATOMIC_ACQUIRE);
	printk("** VMSTAT **\n total: %lu\n used: %lu\n free: %lu\n** VMSTAT END **\n",
	       vmstat.total, used, vmstat.total - used);
}
