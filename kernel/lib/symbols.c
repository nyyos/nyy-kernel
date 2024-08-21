#include <ndk/internal/symbol.h>
#include <ndk/ndk.h>
#include <ndk/kmem.h>
#include <ndk/util.h>

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

#if 0
static char bump_arena[PAGE_SIZE * 64];
static size_t bump_pos = 0;

void *bump_alloc(size_t size)
{
	size = ALIGN_UP(size, 8);
	void *cur = &bump_arena[bump_pos];
	bump_pos += size;
	return cur;
}

#define kmalloc(size) bump_alloc(size)
#endif

void symbols_insert(const char *name, uintptr_t address)
{
	symbol_t *symbol = kmalloc(sizeof(symbol_t));
	memset(symbol, 0x0, sizeof(symbol_t));
	symbol->symname = name;
	symbol->base = address;
	TAILQ_INSERT_TAIL(&symbol_list, symbol, entry);
}

symbol_t *symbols_lookup_address(uintptr_t address)
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

symbol_t *symbols_lookup_name(const char *name)
{
	symbol_t *symbol = NULL;
	TAILQ_FOREACH(symbol, &symbol_list, entry)
	{
		if (strcmp(name, symbol->symname) == 0) {
			return symbol;
		}
	}
	return NULL;
}

static int hex2int(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	return 0;
}

static uintptr_t strtoaddr(const char *str, size_t len)
{
	uintptr_t addr = 0;
	for (size_t i = len, j = 0; j < len; j++, i--) {
		int val = hex2int(str[i - 1]);
		addr += val * (1UL << 4 * j);
	}
	return addr;
}

void symbols_parse_map(char *buf, size_t size)
{
	char *line_start = buf;
	size_t symbol_cnt = 0;
	size_t type_start = -1;
	size_t name_start = -1;
	for (size_t line_pos = 0, pos = 0; pos < size; pos++) {
		if (buf[pos] == '\n') {
			size_t name_len = line_pos - name_start;
			char *name = kmalloc(name_len + 1);
			name[name_len] = '\0';
			strncpy(name, line_start + name_start, name_len);
			size_t addr_len = type_start - 1;
			char *addr_str = kmalloc(addr_len + 1);
			addr_str[addr_len] = '\0';
			strncpy(addr_str, line_start, addr_len);
			uintptr_t addr = strtoaddr(addr_str, addr_len);
			symbols_insert(name, addr);
			symbol_cnt++;
			line_start = &buf[pos + 1];
			line_pos = 0;
			type_start = -1;
			name_start = -1;
			continue;
		} else if (line_start[line_pos] == ' ') {
			if (type_start == -1)
				type_start = line_pos + 1;
			else
				name_start = line_pos + 1;
		}
		line_pos++;
	}
	printk(INFO "added %ld symbols\n", symbol_cnt);
}
