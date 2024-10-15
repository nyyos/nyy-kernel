#include <ndk/ndk.h>
#include <libkern/OSSymbol.h>
#include <libkern/HashMap.h>

#define super OSString

OSDefineMetaClassAndStructors(OSSymbol, OSString);

namespace
{
auto symbol_table = HashMap<std::string, OSSymbol *>();
}

OSSymbol *OSSymbol::fromCStr(const char *cstr)
{
	auto res = symbol_table.lookup(cstr);
	if (res.has_value()) {
		res.value()->retain();
		return res.value();
	}

	auto obj = OSAlloc<OSSymbol>();
	if (!obj || !obj->init())
		return nullptr;

	obj->initFromCStr(cstr);

	symbol_table.insert(obj->m_string, obj);

	return obj;
}

OSSymbol *OSSymbol::fromStr(const OSString *str)
{
	return OSSymbol::fromCStr(str->getCStr());
}

OSSymbol *OSSymbol::fromSym(OSSymbol *str)
{
	assert(str != nullptr);
	str->retain();
	return str;
}

void OSSymbol::free()
{
	assert(symbol_table.remove(m_string));
}

void OSSymbol::initFromCStr(const char *cstr)
{
	super::initFromCStr(cstr);
}
