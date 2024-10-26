#include "DevKit/Common.h"
#include "DevKit/IOCatalog.h"
#include "DevKit/IORegistryEntry.h"
#include "libkern/OSArray.h"
#include "libkern/OSDictionary.h"
#include "libkern/OSString.h"
#include <DevKit/IOPlatformExpert.h>
#include "libkern/OSSymbol.h"
#include <ndk/ndk.h>
#include <libkern/mutex.hpp>
#include <libkern/OSObject.h>
#include <libkern/HashMap.h>
#include <libkern/OSSharedPtr.h>

struct Foo : public OSObject {
	OSDeclareDefaultStructors(Foo);

	using super = OSObject;

    public:
	virtual bool init() override
	{
		if (!super::init())
			return false;
		return true;
	}

	virtual void free() override
	{
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
		return true;
	}

	virtual void free() override
	{
		super::free();
	}
};

OSDefineMetaClassAndStructors(Foo, OSObject);
OSDefineMetaClassAndStructors(Bar, Foo);

void test_fn_cpp()
{
	{
		auto inst = OSAlloc<Foo>();
		inst->init();
		auto inst2 = OSAlloc<Bar>();
		inst2->init();
		auto inst3 = OSAlloc<Bar>();
		inst3->init();

		auto asFoo = inst3->safe_cast<Foo>();
		assert(asFoo != nullptr);
		auto asBar = asFoo->safe_cast<Bar>();
		assert(asBar != nullptr);

		inst3->release();
		inst2->release();
		inst->release();
	}

	{
		char buf[512];
		memset(buf, 0x0, sizeof(buf));
		memcpy(buf, "HELLO WORLD", sizeof("HELLO WORLD"));
		auto str1 = OSString::fromCStr(buf);
		auto str2 = OSString::fromStr(str1);
		assert(str1->getCStr() != str2->getCStr());
		assert(str1->getRefcount() == 1);
		assert(str2->getRefcount() == 1);
		str1->release();
		str2->release();
	}

	{
		auto dict = OSDictionary::makeEmpty();
		auto str_orig = OSString::fromCStr("Hello World");
		{
			dict->set("test", str_orig);
			auto sym = OSSymbol::fromCStr("test");
			assert(sym->getRefcount() == 2);
			sym->release();
			str_orig->release();
			assert(str_orig->getRefcount() == 1);
		}

		OSString *str;
		{
			auto res = dict->get("test");
			assert(res);
			res->retain();
			str = res->safe_cast<OSString>();
			assert(str == str_orig);
			assert(str->getRefcount() == 2);
		}

		assert(dict->getRefcount() == 1);
		dict->release();
		assert(str->getRefcount() == 1);
		str->release();
	}

	{
		auto testmap = HashMap<int, int>();
		testmap.insert(1, 1);

		{
			auto sym1 = OSSymbol::fromCStr("test");
			auto str = OSString::fromCStr("test");
			auto sym2 = OSSymbol::fromStr(str);
			assert(str->getRefcount() == 1);
			str->release();
			assert(sym1 == sym2);
			assert(sym1->getRefcount() == 2);
			sym1->release();
			sym2->release();
		}
	}

	{
		auto mut = lk::Mutex();
		lk::unique_lock<lk::Mutex> uniq;
		{
			const auto guard = lk::lock_guard(mut);
			uniq = lk::unique_lock(mut, lk::adopt_lock_t());
			uniq.release();
		}
	}

	{
		auto arr = OSArray::makeEmpty();
		auto sym = OSSymbol::fromCStr("test");
		arr->insert(sym);
		arr->insert(sym);
		arr->insert(sym);
		arr->insert(sym);
		arr->insert(sym);
		arr->insert(sym);
		arr->insert(sym);
		arr->insert(sym);
		assert(arr->pop() == sym);
		assert(arr->pop() == sym);
		sym->release();
		assert(sym->getRefcount() == 8);
		sym->release();
		sym->release();
		assert(arr->getCount() == 6);
		arr->release();
	}

	{
		auto sym = OSSymbol::fromCStr("test");
		assert(sym->getRefcount() == 1);
		{
			auto ptr = OSMakeShared(OSSymbol::fromCStr("test"));
			assert(sym->getRefcount() == 2);
			assert(sym == ptr.get());
		}
		assert(sym->getRefcount() == 1);
		sym->release();
	}

	auto ps2Pers = OSDictionary::makeEmpty();
	ps2Pers->set(kIoProviderClassKey, "IOACPIDevice");
	ps2Pers->set(kIoClassKey, "PS2Controller");
	auto matchKeys = OSArray::makeEmpty();
	matchKeys->insert("PNP0303");
	ps2Pers->set(kIoACPIMatchKey, *matchKeys);
	auto catalog = IOCatalog::Initialize();
	catalog->addPersonality(*ps2Pers);
	auto root = IORegistryEntry::InitRegistry();
	(volatile void)root;
}

extern "C" {
void test_fn()
{
	test_fn_cpp();
}
}
