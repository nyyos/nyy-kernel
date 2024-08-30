#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/vm.h>
#include <string.h>
#include <sys/queue.h>
#include <assert.h>
#include <nyyconf.h>
#include <buddy_alloc.h>

typedef struct region {
	paddr_t base;
	size_t pagecnt;

	TAILQ_ENTRY(region) entry;

	spinlock_t buddy_lock;
	struct buddy *buddy;

	page_t pages[];
} region_t;

static TAILQ_HEAD(regionlist,
		  region) regionlist = TAILQ_HEAD_INITIALIZER(regionlist);
static TAILQ_HEAD(pagelist, page) pagelist = TAILQ_HEAD_INITIALIZER(pagelist);

vmstat_t vmstat = { 0, 0 };
paddr_t REAL_HHDM_START;

void pm_add_region(paddr_t base, size_t length)
{
	region_t *region;
	size_t i, used;

	used = PAGE_ALIGN_UP(sizeof(region_t) +
			     buddy_sizeof_alignment(length, PAGE_SIZE) +
			     sizeof(page_t) * length / PAGE_SIZE);
	// XXX: is 50 reasonable?
	if ((length - used) < (PAGE_SIZE * 50)) {
		return;
	}

	region = (region_t *)P2V(base).addr;
	region->pagecnt = length / PAGE_SIZE;
	region->base = base;
	TAILQ_INSERT_TAIL(&regionlist, region, entry);

#if 0
	printk(DEBUG "pfn part: %ldB\n",
	       sizeof(region_t) + sizeof(page_t) * length / PAGE_SIZE);
	printk(DEBUG "buddy part: %ldB\n",
	  buddy_sizeof_alignment(length, PAGE_SIZE));
#endif

	vmstat.total += region->pagecnt;

	for (i = 0; i < region->pagecnt; i++) {
		region->pages[i].usage = kPageUseFree;
		region->pages[i].pfn = (region->base.addr + i * PAGE_SIZE) >>
				       12;
	}

	for (i = 0; i < used / PAGE_SIZE; i++) {
		region->pages[i].usage = kPageUseInternal;
	}
	vmstat.used += used / PAGE_SIZE;

	void *metadata = &region->pages[region->pagecnt];
	void *arena = (void *)(base.addr + used);
	assert(((uintptr_t)arena % PAGE_SIZE) == 0);

	region->buddy =
		buddy_init_alignment(metadata, arena, length - used, PAGE_SIZE);

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

static region_t *find_buddy_region(size_t size, irql_t *old)
{
	region_t *elm;
	struct buddy *buddy;
	TAILQ_FOREACH(elm, &regionlist, entry)
	{
		buddy = elm->buddy;
		*old = spinlock_acquire(&elm->buddy_lock, IRQL_HIGH);
		if (buddy_arena_free_size(buddy) < size) {
			spinlock_release(&elm->buddy_lock, *old);
			continue;
		}
		return elm;
	}
	panic("OOM");
	return nullptr;
}

static inline page_t *pm_lookup_with_region(paddr_t paddr, region_t *region)
{
	return &region->pages[(paddr.addr - region->base.addr) / PAGE_SIZE];
}

page_t *pm_allocate_n(size_t n, short usage)
{
	irql_t irql;
	region_t *region = find_buddy_region(n * PAGE_SIZE, &irql);
	if (region == nullptr)
		return nullptr;
	void *mem = buddy_malloc(region->buddy, n * PAGE_SIZE);
	assert(mem != nullptr);
	__atomic_fetch_add(&vmstat.used, n, __ATOMIC_RELAXED);
	spinlock_release(&region->buddy_lock, irql);
	return pm_lookup_with_region(PADDR(mem), region);
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
	region_t *region = pm_lookup_region(PG2P(pages));
	irql_t irql = spinlock_acquire(&region->buddy_lock, IRQL_HIGH);
	buddy_safe_free(region->buddy, (void *)PG2P(pages).addr, n * PAGE_SIZE);
	__atomic_fetch_sub(&vmstat.used, n, __ATOMIC_RELAXED);
	spinlock_release(&region->buddy_lock, irql);
}

void vmstat_dump()
{
	size_t used = __atomic_load_n(&vmstat.used, __ATOMIC_ACQUIRE);
	printk("** VMSTAT **\n total: %lu\n used: %lu\n free: %lu\n** VMSTAT END **\n",
	       vmstat.total, used, vmstat.total - used);
}
