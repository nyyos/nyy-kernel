#include "DevKit/Common.h"
#include "libkern/OSArray.h"
#include "libkern/OSMetaClass.h"
#include "libkern/OSSymbol.h"
#include <DevKit/IOCatalog.h>

#define super OSObject
OSDefineMetaClassAndStructors(IOCatalog, OSObject);

IOCatalog *gIoCatalog;
OSSymbol *gIoProviderClassKey;

IOCatalog *IOCatalog::Initialize()
{
	gIoCatalog = OSAlloc<IOCatalog>();
	gIoCatalog->init();

	gIoProviderClassKey = OSSymbol::fromCStr(kIoProviderClassKey);

	return gIoCatalog;
}

bool IOCatalog::init()
{
	if (!super::init())
		return false;

	personalities = OSDictionary::makeEmpty();

	return true;
}

void IOCatalog::free()
{
	personalities->release();
}

void IOCatalog::addPersonality(OSDictionary *dict)
{
	const OSSymbol *sym;
	sym = OSDynamicCast<OSSymbol>(dict->get(gIoProviderClassKey));
	if (!sym) {
		return;
	}

	OSArray *arr = (OSArray *)personalities->get(sym);
	if (arr) {
		arr->insert(dict);
	} else {
		auto ret = OSArray::makeEmpty();
		auto arr = ret.get();
		arr->insert(dict);
		personalities->set(sym, arr);
		assert(arr->getRefcount() > 1);
	}
}

void IOCatalog::printPersonalities() const
{
	for (const auto &[key, value] : *personalities->map) {
		printk("%s:\n", key->getCStr());
		auto arr = (OSArray *)value;
		for (int i = 0; i < arr->getCount(); i++) {
			auto entry = (OSDictionary *)arr->getEntry(i);
			auto sym = (OSSymbol *)entry->get(kIoClassKey);

			printk(" - %s\n", sym->getCStr());
		}
	}
}
