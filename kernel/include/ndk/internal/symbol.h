#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/queue.h>

typedef struct symbol {
	const char *symname;
	uintptr_t base;

	TAILQ_ENTRY(symbol) entry;
} symbol_t;

void symbols_init(size_t offset);
size_t symbols_offset();
void symbols_insert(const char *name, uintptr_t address);
symbol_t *symbols_lookup(uintptr_t address);
