#include "libkern/OSObject.h"
#include <libkern/OSDictionary.h>

#define super OSObject

OSDefineMetaClassAndStructors(OSDictionary, OSObject);

OSSharedPtr<OSDictionary> OSDictionary::makeEmpty()
{
	auto obj = OSMakeShared<OSDictionary>();
	if (!obj->init()) {
		obj->release();
		return nullptr;
	}
	return obj;
}

OSSharedPtr<OSDictionary> OSDictionary::makeWithSize(size_t size)
{
	auto obj = OSMakeShared<OSDictionary>();
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
	map = new HashMap<const OSSymbol *, OSMetaClassBase *>(size);
	if (map)
		return true;
	return false;
}

void OSDictionary::free()
{
	if (map) {
		for (const auto &[key, value] : *map) {
			map->remove(key);
			const_cast<OSSymbol *>(key)->release();
			value->release();
		}
	}

	super::free();
}

bool OSDictionary::set(const OSSymbol *sym, OSMetaClassBase *value)
{
	auto ret = map->update(sym, value);
	value->retain();
	if (!ret.has_value())
		const_cast<OSSymbol *>(sym)->retain();
	else
		ret.value()->release();
	return true;
}

bool OSDictionary::set(const char *str, OSMetaClassBase *value)
{
	auto key = OSSymbol::fromCStr(str);
	auto res = set(key, value);
	key->release();
	return res;
}

bool OSDictionary::set(const char *key, const char *value)
{
	auto val = OSSymbol::fromCStr(value);
	auto res = set(key, val);
	val->release();
	return res;
}

OSObject *OSDictionary::get(const char *str)
{
	const auto sym = OSSymbol::fromCStr(str);
	auto res = OSDictionary::get(sym);
	sym->release();
	return res;
}

OSObject *OSDictionary::get(const OSSymbol *sym)
{
	auto res = map->lookup(sym);
	if (res.has_value()) {
		auto val = res.value();
		return static_cast<OSObject *>(val);
	}
	return nullptr;
}

bool OSDictionary::unset(const OSSymbol *sym)
{
	auto old_maybe = map->lookup(sym);
	if (!old_maybe.has_value()) {
		return false;
	}
	auto old = old_maybe.value();
	auto ret = map->remove(sym);
	old->release();
	const_cast<OSSymbol *>(sym)->release();
	return ret;
}
