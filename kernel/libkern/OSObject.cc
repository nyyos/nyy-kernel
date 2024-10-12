#include <atomic>
#include <ndk/ndk.h>
#include <libkern/OSObject.h>

OSObject::MetaClass OSObject ::gMetaClass;
const OSMetaClass *const OSObject ::metaClass = &OSObject ::gMetaClass;
const OSMetaClass *const OSObject ::superClass = nullptr;

OSObject ::OSObject(const OSMetaClass *meta)
	: refcnt{ 1 }
{
}

OSObject::OSObject()
	: refcnt{ 1 }
{
}

OSObject ::~OSObject()
{
}

const OSMetaClass *OSObject ::getMetaClass() const
{
	return &gMetaClass;
};

OSObject *OSObject ::MetaClass ::alloc() const
{
	return nullptr;
}

OSObject::MetaClass ::MetaClass()
	: OSMetaClass("OSObject", OSObject::superClass, sizeof(OSObject))
{
}

void OSObject::release(int freeWhen)
{
	refcnt.fetch_sub(1, std::memory_order_release);
	if (refcnt.load() == freeWhen)
		free();
}

void OSObject::retain()
{
	refcnt.fetch_add(1, std::memory_order_acquire);
}

void OSObject::free()
{
	printk(DEBUG "free() object base: %p\n", this);
	delete this;
}

bool OSObject::init()
{
	return true;
}
