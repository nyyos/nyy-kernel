#pragma once

#include <ndk/obj.h>

typedef struct event {
	obj_header_t hdr;
} event_t;

void event_init(event_t *event, bool notif, bool signaled);
int event_reset(event_t *event);
int event_signal(event_t *event);
