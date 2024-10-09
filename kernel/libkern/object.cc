#include <atomic>
#include <lk/base.hpp>
#include <ndk/ndk.h>

Object::Object()
{
	refcnt = 1;
}

void Object::release(int freeWhen)
{
	refcnt.fetch_sub(1, std::memory_order_release);
	if (refcnt.load() == freeWhen)
		free();
}

void Object::retain()
{
	refcnt.fetch_add(1, std::memory_order_acquire);
}

void Object::free()
{
	printk(DEBUG "free() object base\n");
	delete this;
}

bool Object::init()
{
	return true;
}
