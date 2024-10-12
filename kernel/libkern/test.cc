#include <ndk/ndk.h>
#include <libkern/mutex.hpp>
#include <libkern/OSObject.h>

#include <DevKit/IORegistry.h>

struct Foo : public OSObject {
	OSDeclareDefaultStructors(Foo);

	using super = OSObject;

    public:
	virtual bool init() override
	{
		if (!super::init())
			return false;
		printk("init foo\n");
		return true;
	}

	virtual void free() override
	{
		printk("free foo\n");
		super::free();
	}
};

struct Bar final : public Foo {
	OSDeclareDefaultStructors(Bar);

	using super = Foo;

    public:
	virtual bool init() override
	{
		if (!super::init())
			return false;
		printk("init bar\n");
		return true;
	}

	virtual void free() override
	{
		printk("free bar\n");
		super::free();
	}
};

OSDefineMetaClassAndStructors(Foo, OSObject);
OSDefineMetaClassAndStructors(Bar, Foo);

void test_fn_cpp()
{
	auto inst = Foo::gMetaClass.alloc();
	inst->init();
	auto inst2 = Bar::gMetaClass.alloc();
	inst2->init();
	auto inst3 = Bar::gMetaClass.alloc();
	inst3->init();

	auto asFoo = inst3->safe_cast<Foo>();
	assert(asFoo != nullptr);
	auto asBar = asFoo->safe_cast<Bar>();
	assert(asBar != nullptr);

	printk("asFoo:%s\n", asFoo->getMetaClass()->getClassName());
	printk("asBar:%s\n", asBar->getMetaClass()->getClassName());

	inst3->release();
	inst2->release();
	inst->release();

	auto mut = lk::Mutex();
	lk::unique_lock<lk::Mutex> uniq;
	{
		const auto guard = lk::lock_guard(mut);
		uniq = lk::unique_lock(mut, lk::adopt_lock_t());
		uniq.release();
	}
}

extern "C" {
void test_fn()
{
	test_fn_cpp();
}
}
