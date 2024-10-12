#include <libkern/OSString.h>

OSDefineMetaClassAndStructors(OSString, OSObject);

OSString *OSString::fromCStr(const char *cstr)
{
	auto obj = static_cast<OSString *>(OSString::gMetaClass.alloc());
	if (!obj || !obj->init())
		return nullptr;

	obj->initFromCStr(cstr);

	return obj;
}

OSString *OSString::fromStr(const OSString *str)
{
	auto obj = static_cast<OSString *>(OSString::gMetaClass.alloc());
	if (!obj->init())
		return nullptr;

	obj->initFromStr(str);

	return obj;
}

void OSString::initFromCStr(const char *cstr)
{
	m_string = std::string(cstr);
}

void OSString::initFromStr(const OSString *str)
{
	m_string = std::string(str->m_string);
}
