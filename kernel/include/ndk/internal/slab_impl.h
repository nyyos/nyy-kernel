#pragma once

#include "ndk/mutex.h"
#include <stddef.h>
#include <sys/queue.h>

#include <ndk/port.h>

#define KMEM_NAME_MAX 64
#define KMEM_SMALL_MAX (PAGE_SIZE / 16)
#define KMEM_ALIGN 8
#ifdef _LP64
#define KMEM_SECOND_ALIGN 16
#else
#define KMEM_SKMEM_SECOND_ALIGN KMEM_ALIGN
#endif

#define KMEM_NOSLEEP (1 << 0) // if sleeping is allowed

typedef struct kmem_cache kmem_cache_t;
typedef struct kmem_slab kmem_slab_t;

typedef struct kmem_bufctl {
	SLIST_ENTRY(kmem_bufctl) entry;
} kmem_bufctl_t;

typedef struct kmem_large_bufctl {
	kmem_bufctl_t bufctl;
	kmem_slab_t *slab;
	void *base;
} kmem_large_bufctl_t;

typedef struct kmem_slab {
	SLIST_HEAD(freelist, kmem_bufctl) freelist;
	LIST_ENTRY(kmem_slab) entry;
	size_t refcnt;
	kmem_cache_t *parent_cache;
	void *base;
} kmem_slab_t;

typedef struct kmem_cache {
	char name[KMEM_NAME_MAX + 1];
	size_t slabcap;
	size_t align;
	size_t chunksize;
	size_t bufsize;
	size_t slabsize;

	int (*constructor)(void *, void *);
	void (*destructor)(void *, void *);
	void *ctx;

	mutex_t mutex;

	SLIST_HEAD(buflist, kmem_bufctl) buflist;
	LIST_HEAD(slablist, kmem_slab) slablist;
	LIST_HEAD(full_slablist, kmem_slab) full_slablist;
} kmem_cache_t;

void kmem_slab_init();
// might be used in ports like Linux and tests
void kmem_slab_cleanup();
