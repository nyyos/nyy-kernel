#include <cstddef>
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

struct ArrayAlloc {
	size_t size;
	alignas(std::max_align_t) char content[];
};

void *operator new[](unsigned long sz)
{
	auto p = static_cast<ArrayAlloc *>(kmalloc(sz + sizeof(ArrayAlloc)));
	memset(p->content, 0x0, sz);
	p->size = sz;
	return p->content;
}

void operator delete[](void *ptr, unsigned long sz)
{
	auto p = reinterpret_cast<ArrayAlloc *>((uintptr_t)ptr -
						offsetof(ArrayAlloc, content));
	kfree(p, p->size + sizeof(ArrayAlloc));
}

void operator delete[](void *ptr)
{
	auto p = reinterpret_cast<ArrayAlloc *>((uintptr_t)ptr -
						offsetof(ArrayAlloc, content));
	kfree(p, p->size + sizeof(ArrayAlloc));
}
