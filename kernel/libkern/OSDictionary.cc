#include "libkern/OSObject.h"
#include <libkern/OSDictionary.h>

#define super OSObject

OSDefineMetaClassAndStructors(OSDictionary, OSObject);

OSDictionary *OSDictionary::makeEmpty()
{
	auto obj = OSAlloc<OSDictionary>();
	if (!obj->init()) {
		obj->release();
		return nullptr;
	}
	return obj;
}

OSDictionary *OSDictionary::makeWithSize(size_t size)
{
	auto obj = OSAlloc<OSDictionary>();
	if (!obj->initWithSize(size)) {
		obj->release();
		return nullptr;
	}
	return obj;
}

bool OSDictionary::init()
{
	return initWithSize(16);
}

bool OSDictionary::initWithSize(size_t size)
{
	if (!super::init())
		return false;
	map = new HashMap<OSSymbol *, OSMetaClassBase *>(size);
	if (map)
		return true;
	return false;
}

void OSDictionary::free()
{
	if (map) {
		for (const auto &[key, value] : *map) {
			map->remove(key);
			key->release();
			value->release();
		}
	}

	super::free();
}

bool OSDictionary::set(OSSymbol *sym, OSMetaClassBase *value)
{
	auto ret = map->update(sym, value);
	value->retain();
	if (!ret.has_value())
		sym->retain();
	else
		ret.value()->release();
	return true;
}

bool OSDictionary::set(std::string_view str, OSMetaClassBase *value)
{
	auto key = OSSymbol::fromCStr(str.data());
	auto res = set(key, value);
	key->release();
	return res;
}

OSObject *OSDictionary::get(const char *str)
{
	auto sym = OSSymbol::fromCStr(str);
	auto res = OSDictionary::get(sym);
	sym->release();
	return res;
}

OSObject *OSDictionary::get(OSSymbol *sym)
{
	auto res = map->lookup(sym);
	if (res.has_value()) {
		auto val = res.value();
		val->retain();
		return static_cast<OSObject *>(val);
	}
	return nullptr;
}

bool OSDictionary::unset(OSSymbol *sym)
{
	auto old_maybe = map->lookup(sym);
	if (!old_maybe.has_value()) {
		sym->release();
		return false;
	}
	auto old = old_maybe.value();
	auto ret = map->remove(sym);
	old->release();
	sym->release(1);
	return ret;
}
