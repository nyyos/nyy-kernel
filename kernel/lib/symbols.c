#include <ndk/internal/symbol.h>
#include <ndk/ndk.h>
#include <ndk/kmem.h>

#include <string.h>
#include <sys/queue.h>

static TAILQ_HEAD(symbol_list,
		  symbol) symbol_list = TAILQ_HEAD_INITIALIZER(symbol_list);
static size_t symbol_offset = 0;

void symbols_init(size_t offset)
{
	symbol_offset = offset;
}

size_t symbols_offset()
{
	return symbol_offset;
}

void symbols_insert(const char *name, uintptr_t address)
{
	symbol_t *symbol = kmalloc(sizeof(symbol_t));
	memset(symbol, 0x0, sizeof(symbol_t));
	symbol->symname = name;
	symbol->base = address;
	TAILQ_INSERT_TAIL(&symbol_list, symbol, entry);
}

symbol_t *symbols_lookup(uintptr_t address)
{
	address -= symbol_offset;
	symbol_t *symbol = NULL;
	symbol_t *best_match = NULL;
	TAILQ_FOREACH(symbol, &symbol_list, entry)
	{
		if (address > symbol->base) {
			best_match = symbol;
		} else if (symbol->base > address)
			break;
	}
	return best_match;
}
