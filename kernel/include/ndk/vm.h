#pragma once

#include <ndk/port.h>
#include <ndk/util.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

#define PAGE_ALIGN_UP(addr) ALIGN_UP(addr, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(addr) ALIGN_DOWN(addr, PAGE_SIZE)

#ifndef P2V
#define P2V(paddr) (vaddr)(paddr + REAL_HHDM_START)
#endif

#define PG2P(page) (page->pfn << 12)

typedef uintptr_t paddr;
typedef uintptr_t vaddr;

extern paddr REAL_HHDM_START;

enum pageuse {
  kPageUseFree = 1,
  kPageUseInternal,
};

typedef struct page {
  paddr pfn;
  short usage;
  TAILQ_ENTRY(page) entry;
} page_t;

/*
 * ================================
 * |   Physical Memory Manager    |
 * ================================
 */

void pm_initialize();
void pm_add_region(paddr base, size_t length);
page_t *pm_allocate();
void pm_free(page_t *page);
