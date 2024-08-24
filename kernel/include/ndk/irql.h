#pragma once

#include <sys/queue.h>
#include <stddef.h>
#include <stdint.h>
#include <ndk/port.h>

typedef uint8_t irql_t;

irql_t irql_raise(irql_t level);
void irql_lower(irql_t level);
/* provided by port */
irql_t irql_current();
void irql_set(irql_t irql);
