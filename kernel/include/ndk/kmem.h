#pragma once

#include <stddef.h>

void kmem_init();
void *kmalloc(size_t size);
void kfree(void *ptr, size_t size);
void *kcalloc(size_t count, size_t size);
