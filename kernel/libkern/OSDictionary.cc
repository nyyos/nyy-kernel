#include "libkern/OSObject.h"
#include <libkern/OSDictionary.h>

#define super OSObject

OSDefineMetaClassAndStructors(OSDictionary, OSObject);

bool OSDictionary::init()
{
	if (!super::init())
		return false;

	map = new HashMap<OSSymbol *, OSObject *>(16);
	return true;
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

void OSDictionary::set(OSSymbol *sym, OSObject *value)
{
	map->update(sym, value);
}

void OSDictionary::set(std::string_view str, OSObject *value)
{
	map->update(OSSymbol::fromCStr(str.data()), value);
}

OSObject *OSDictionary::get(OSSymbol *sym)
{
	auto res = map->lookup(sym);
	if (res.has_value()) {
		auto val = res.value();
		val->retain();
		return val;
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
