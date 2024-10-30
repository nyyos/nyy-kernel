#include "ndk/obj.h"

void obj_init(obj_header_t *hdr)
{
	hdr->refcnt = 1;
}

void obj_retain(obj_header_t *hdr)
{
	__atomic_fetch_add(&hdr->refcnt, 1, __ATOMIC_ACQUIRE);
}

void obj_release(obj_header_t *hdr)
{
	if (1 == __atomic_fetch_sub(&hdr->refcnt, 1, __ATOMIC_RELEASE)) {
		hdr->ops->free(hdr);
	}
}
