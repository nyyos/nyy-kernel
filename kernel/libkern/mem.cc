#include <ndk/kmem.h>
#include <string.h>
#include <ndk/ndk.h>

void *operator new(unsigned long sz)
{
	auto p = kmalloc(sz);
	memset(p, 0x0, sz);
	return p;
}

void operator delete(void *ptr, unsigned long sz)
{
	kfree(ptr, sz);
}

void *operator new[](unsigned long sz)
{
#if UINTPTR_MAX != UINT64_MAX
#error "arch change this"
#endif
	auto sz_actual = ALIGN_UP(sz + sizeof(size_t), 16);
	auto p = kmalloc(sz_actual);
	memset(p, 0x0, sz);
	auto tag = (size_t *)p;
	*tag = sz_actual;
	tag++;
	return (void *)ALIGN_UP((uintptr_t)++tag, 16);
}

void operator delete[](void *ptr, unsigned long sz)
{
	size_t *p = static_cast<size_t *>(ptr);
	p -= 2;
	kfree(p, *p);
}

void operator delete[](void *ptr)
{
	size_t *p = static_cast<size_t *>(ptr);
	p -= 2;
	kfree(p, *p);
}
