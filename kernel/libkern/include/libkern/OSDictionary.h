#pragma once

#include "libkern/OSSharedPtr.h"
#include <libkern/OSMetaClass.h>
#include <libkern/OSObject.h>
#include <libkern/HashMap.h>
#include <libkern/OSSymbol.h>

class IOCatalog;

class OSDictionary : public OSObject {
	OSDeclareDefaultStructors(OSDictionary);

    public:
	static OSSharedPtr<OSDictionary> makeEmpty();
	static OSSharedPtr<OSDictionary> makeWithSize(size_t size);

	virtual bool init() override;
	virtual bool initWithSize(size_t size);
	virtual void free() override;

	virtual OSObject *get(const char *str);
	virtual OSObject *get(const OSSymbol *sym);
	virtual bool unset(const OSSymbol *sym);

	virtual bool set(const char *str, OSMetaClassBase *value);
	virtual bool set(const OSSymbol *sym, OSMetaClassBase *value);

	virtual bool set(const char *key, const char *value);

    protected:
	friend class IOCatalog;
	HashMap<const OSSymbol *, OSMetaClassBase *> *map;
};
