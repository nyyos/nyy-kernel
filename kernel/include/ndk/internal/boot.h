#pragma once

#include <stddef.h>
#include <ndk/addr.h>

void remap_memmap_entry(uintptr_t base, size_t length, int cache);
void remap_kernel(paddr_t paddr, vaddr_t vaddr);
