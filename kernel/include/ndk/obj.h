#pragma once

#include <stddef.h>

typedef struct obj_header obj_header_t;

typedef struct obj_ops {
	void (*free)(obj_header_t *hdr);
} obj_ops_t;

typedef struct obj_header {
	size_t refcnt;
	obj_ops_t *ops;
} obj_header_t;

void obj_init(obj_header_t *hdr);
void obj_retain(obj_header_t *hdr);
void obj_release(obj_header_t *hdr);
