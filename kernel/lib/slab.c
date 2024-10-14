#include <assert.h>
#include <string.h>

#include <ndk/internal/slab_impl.h>
#include <ndk/port.h>
#include <ndk/addr.h>
#include <ndk/util.h>
#include <ndk/kmem.h>
#include <ndk/vm.h>
#include <ndk/ndk.h>

#define KMEM_BOOSTRAP (1 << 0)
#define KMEM_MAXPAGES 8

#ifndef __KERNEL__
#ifndef NYYLINUX
void *vm_kalloc(size_t count, int flags)
{
	(void)flags;
	void *ret = NULL;
	posix_memalign(&ret, PAGE_SIZE, PAGE_SIZE * count);
	return ret;
}

void vm_kfree(void *addr, size_t count)
{
	free(addr);
}
#endif
#endif

static kmem_cache_t *slab_cache;
static kmem_cache_t *bufctl_cache;

static kmem_slab_t *small_slab_create(kmem_cache_t *cp, int flags)
{
	kmem_slab_t *sp;
	void *pg;

	pg = vm_kalloc(1, flags);
	sp = (kmem_slab_t *)((uintptr_t)pg + PAGE_SIZE - sizeof(kmem_slab_t));
	SLIST_INIT(&sp->freelist);
	sp->refcnt = 0;
	sp->parent_cache = cp;
	sp->base = NULL;
	for (size_t i = 0; i < cp->slabcap; i++) {
		SLIST_INSERT_HEAD(
			&sp->freelist,
			(kmem_bufctl_t *)((uintptr_t)pg + (i * cp->chunksize)),
			entry);
	}

	return sp;
}

static kmem_slab_t *large_slab_create(kmem_cache_t *cp, int flags)
{
	kmem_slab_t *sp;
	sp = kmem_cache_alloc(slab_cache, flags);
	assert(sp);
	SLIST_INIT(&sp->freelist);
	sp->refcnt = 0;
	sp->parent_cache = cp;

	void *base = vm_kalloc(cp->slabsize, flags);
	sp->base = base;

	for (size_t i = 0; i < cp->slabcap; i++) {
		kmem_large_bufctl_t *bp = kmem_cache_alloc(bufctl_cache, flags);
		bp->base = (void *)((uintptr_t)base + cp->chunksize * i);
		bp->slab = sp;
		SLIST_INSERT_HEAD(&sp->freelist, &bp->bufctl, entry);
	}

	return sp;
}

static void *slab_alloc(kmem_slab_t *sp)
{
	kmem_bufctl_t *bufctl = SLIST_FIRST(&sp->freelist);
	SLIST_REMOVE_HEAD(&sp->freelist, entry);
	sp->refcnt++;
	if (sp->refcnt == sp->parent_cache->slabcap) {
		LIST_REMOVE(sp, entry);
		LIST_INSERT_HEAD(&sp->parent_cache->full_slablist, sp, entry);
	}
	if (sp->base == NULL)
		return (void *)bufctl;
	SLIST_INSERT_HEAD(&sp->parent_cache->buflist, bufctl, entry);
	return ((kmem_large_bufctl_t *)bufctl)->base;
}

static void slab_free(kmem_slab_t *sp, kmem_bufctl_t *bufctl)
{
	assert(sp);
	assert(bufctl);
	if (sp->refcnt == sp->parent_cache->slabcap) {
		LIST_REMOVE(sp, entry);
		LIST_INSERT_HEAD(&sp->parent_cache->slablist, sp, entry);
	}
	sp->refcnt--;

	if (sp->base != NULL) {
		SLIST_REMOVE(&sp->parent_cache->buflist, bufctl, kmem_bufctl,
			     entry);
	}

	SLIST_INSERT_HEAD(&sp->freelist, bufctl, entry);
}

void *kmem_cache_alloc(kmem_cache_t *cp, int kmflags)
{
	if (LIST_EMPTY(&cp->slablist)) {
		kmem_slab_t *slab;
		if (cp->chunksize <= KMEM_SMALL_MAX) {
			slab = small_slab_create(cp, kmflags);
		} else {
			slab = large_slab_create(cp, kmflags);
		}
		LIST_INSERT_HEAD(&cp->slablist, slab, entry);
	}

	kmem_slab_t *sp = LIST_FIRST(&cp->slablist);
	assert(sp->refcnt < cp->slabcap);
	void *obj = slab_alloc(sp);
	if (cp->constructor != NULL) {
		assert(cp->constructor(obj, cp->ctx) == 0);
	}

	return obj;
}

void kmem_cache_free(kmem_cache_t *cp, void *obj)
{
	kmem_slab_t *sp = NULL;
	kmem_bufctl_t *bp;
	if (cp->destructor != NULL)
		cp->destructor(obj, cp->ctx);
	if (cp->chunksize <= KMEM_SMALL_MAX) {
		sp = (kmem_slab_t *)(ALIGN_UP((uintptr_t)obj + 1, PAGE_SIZE) -
				     sizeof(kmem_slab_t));
		bp = (kmem_bufctl_t *)obj;
	} else {
		kmem_bufctl_t *elm;
		SLIST_FOREACH(elm, &cp->buflist, entry)
		{
			kmem_large_bufctl_t *lbp = (kmem_large_bufctl_t *)elm;
			if (lbp->base == obj) {
				sp = lbp->slab;
				bp = elm;
				break;
			}
		}
		assert(sp != NULL);
	}

	slab_free(sp, bp);
}

kmem_cache_t *kmem_cache_create(const char *name, size_t bufsize, size_t align,
				int (*constructor)(void *, void *),
				void (*destructor)(void *, void *), void *ctx,
				int kmflags)
{
	size_t chunksize;

	// check if P2
	assert(align == 0 || P2CHECK(align));
	assert(bufsize != 0);

	if (align == 0) {
		if (ALIGN_UP(bufsize, KMEM_ALIGN) >= KMEM_SECOND_ALIGN)
			align = KMEM_SECOND_ALIGN;
		else
			align = KMEM_ALIGN;
	} else if (align < KMEM_ALIGN)
		align = KMEM_ALIGN;

	kmem_cache_t *cp;
	if (kmflags & KMEM_BOOSTRAP || 1)
		cp = vm_kalloc(ALIGN_UP(sizeof(kmem_cache_t), PAGE_SIZE) /
				       PAGE_SIZE,
			       0);

	strncpy(cp->name, name, KMEM_NAME_MAX);
	cp->bufsize = bufsize;
	cp->align = align;
	cp->constructor = constructor;
	cp->destructor = destructor;
	cp->ctx = ctx;

	LIST_INIT(&cp->full_slablist);
	LIST_INIT(&cp->slablist);

	if (bufsize <= KMEM_SMALL_MAX) {
		chunksize = ALIGN_UP(bufsize, align);
		// XXX: determine chunksize for good color
		cp->chunksize = chunksize;
		cp->slabcap = (PAGE_SIZE - sizeof(kmem_slab_t)) / cp->chunksize;
		cp->slabsize = 1;
	} else {
		chunksize = cp->chunksize = ALIGN_UP(bufsize, align);
		cp->slabsize = -1;
		size_t pages;
		// XXX: check where min waste
		for (pages = 1; pages < KMEM_MAXPAGES + 1; pages++) {
			// aim for 16 chunks minimum
			if ((pages * PAGE_SIZE / cp->chunksize) >= 16) {
				cp->slabcap =
					(pages * PAGE_SIZE / cp->chunksize);
				cp->slabsize = pages;
				break;
			}
		}
		assert(cp->slabsize != -1);
	}

	return cp;
}

int kmem_cache_destroy(kmem_cache_t *cp)
{
	if (!LIST_EMPTY(&cp->full_slablist)) {
		printk("not empty: %p\n", LIST_FIRST(&cp->full_slablist));
		return -1;
	}

	while (!LIST_EMPTY(&cp->slablist)) {
		kmem_slab_t *sp = LIST_FIRST(&cp->slablist);
		if (sp->refcnt != 0)
			return -1;
		LIST_REMOVE(sp, entry);
		if (sp->base == NULL) {
			vm_kfree((void *)ALIGN_DOWN((uintptr_t)sp, PAGE_SIZE),
				 cp->slabsize);
		} else {
			while (!SLIST_EMPTY(&sp->freelist)) {
				kmem_bufctl_t *elm = SLIST_FIRST(&sp->freelist);
				SLIST_REMOVE_HEAD(&sp->freelist, entry);
				kmem_cache_free(bufctl_cache, elm);
			}

			vm_kfree(sp->base, cp->slabsize);
			kmem_cache_free(slab_cache, sp);
		}
	}

	vm_kfree(cp, ALIGN_UP(sizeof(kmem_cache_t), PAGE_SIZE) / PAGE_SIZE);

	return 0;
}

void kmem_slab_init()
{
	slab_cache = kmem_cache_create("slab_cache", sizeof(kmem_slab_t), 0,
				       NULL, NULL, NULL, 0);
	bufctl_cache = kmem_cache_create("bufctl_cache",
					 sizeof(kmem_large_bufctl_t), 0, NULL,
					 NULL, NULL, 0);
}

void kmem_slab_cleanup()
{
	kmem_cache_destroy(slab_cache);
	kmem_cache_destroy(bufctl_cache);
}
