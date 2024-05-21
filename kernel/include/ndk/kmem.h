#pragma once

#include <stddef.h>

void kmem_init();
void *kmalloc(size_t size);
void kfree(void *ptr);
