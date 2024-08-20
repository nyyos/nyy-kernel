#pragma once

#include <stddef.h>

typedef struct kmem_cache kmem_cache_t;

void kmem_init();

kmem_cache_t *kmem_cache_create(const char *name, size_t bufsize, size_t align,
				int (*constructor)(void *, void *),
				void (*destructor)(void *, void *), void *ctx,
				int kmflags);

// returns 0 on success
int kmem_cache_destroy(kmem_cache_t *cp);

void *kmem_cache_alloc(kmem_cache_t *cp, int kmflags);
void kmem_cache_free(kmem_cache_t *cp, void *obj);

// on init time caches of various sizes are created,
// the kmalloc suite operate upon those
void *kmalloc(size_t size);
void kfree(void *ptr, size_t size);
void *kcalloc(size_t count, size_t size);
