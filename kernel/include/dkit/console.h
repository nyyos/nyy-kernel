#pragma once

#include <stddef.h>
#include <sys/queue.h>

typedef struct console {
	const char *name;
	void (*write)(struct console *, const char *msg, size_t sz);
	unsigned int flags;

	TAILQ_ENTRY(console) queue_entry;
} console_t;

void console_add(console_t *console);
void console_remove(console_t *console);
