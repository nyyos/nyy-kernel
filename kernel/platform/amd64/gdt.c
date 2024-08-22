#include <stdint.h>
#include <string.h>

#include <ndk/ndk.h>
#include "gdt.h"

typedef struct [[gnu::packed]] gdtr {
	uint16_t limit;
	uint64_t base;
} gdtr_t;

typedef struct [[gnu::packed]] gdt_tss_entry {
	uint16_t length;
	uint16_t low;
	uint8_t mid;
	uint8_t access;
	uint8_t flags;
	uint8_t high;
	uint32_t upper;
	uint32_t rsv;
} gdt_tss_entry_t;

typedef struct [[gnu::packed]] gdt_entry {
	uint16_t limit;
	uint16_t low;
	uint8_t mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t high;
} gdt_entry_t;

typedef struct [[gnu::packed]] gdt {
	gdt_entry_t entries[4];
	gdt_tss_entry_t tss;
} gdt_t;

static gdt_t g_gdt;
static spinlock_t g_tsslock;

static void gdt_make_entry(gdt_t *gdt, int index, uint32_t base, uint16_t limit,
			   uint8_t access, uint8_t granularity)
{
	gdt_entry_t *ent = &gdt->entries[index];
	ent->limit = limit;
	ent->access = access;
	ent->granularity = granularity;
	ent->low = (uint16_t)base;
	ent->mid = (uint8_t)(base >> 16);
	ent->high = (uint8_t)(base >> 24);
}

static void gdt_tss_entry(gdt_t *gdt)
{
	gdt->tss.length = 104;
	gdt->tss.access = 0x89;
	gdt->tss.flags = 0;
	gdt->tss.rsv = 0;

	gdt->tss.low = 0;
	gdt->tss.mid = 0;
	gdt->tss.high = 0;
	gdt->tss.upper = 0;
}

void cpu_gdt_init()
{
	SPINLOCK_INIT(&g_tsslock);
	memset(&g_gdt, 0x0, sizeof(gdt_t));
	gdt_make_entry(&g_gdt, kGdtNull, 0, 0, 0, 0);
	gdt_make_entry(&g_gdt, kGdtKernelCode, 0, 0, 0x9a, 0x20);
	gdt_make_entry(&g_gdt, kGdtKernelData, 0, 0, 0x92, 0);
	gdt_make_entry(&g_gdt, kGdtUserData, 0, 0, 0xf2, 0);
	gdt_make_entry(&g_gdt, kGdtUserCode, 0, 0, 0xfa, 0x20);
	gdt_tss_entry(&g_gdt);
}

extern void cpu_gdt_load_(gdtr_t *gdtr, int cs, int ss);
void cpu_gdt_load()
{
	gdtr_t gdtr = { .limit = sizeof(gdt_t) - 1, .base = (uint64_t)&g_gdt };
	cpu_gdt_load_(&gdtr, kGdtKernelCode * 8, kGdtKernelData * 8);
}

void cpu_tss_load(tss_t *tss)
{
	irql_t old = spinlock_acquire(&g_tsslock, IRQL_HIGH);
	uint64_t tss_addr = (uint64_t)tss;
	g_gdt.tss.low = (uint16_t)tss_addr;
	g_gdt.tss.mid = (uint8_t)(tss_addr >> 16);
	g_gdt.tss.high = (uint8_t)(tss_addr >> 24);
	g_gdt.tss.upper = (uint32_t)(tss_addr >> 32);
	g_gdt.tss.flags = 0;
	g_gdt.tss.access = 0b10001001;
	g_gdt.tss.rsv = 0;
	asm volatile("ltr %0" ::"rm"(kGdtTss * 8) : "memory");
	spinlock_release(&g_tsslock, old);
}
