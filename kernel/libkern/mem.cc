#include <ndk/kmem.h>

void *operator new(unsigned long sz)
{
	return kmalloc(sz);
}

void operator delete(void *ptr, unsigned long sz)
{
	return kfree(ptr, sz);
}
