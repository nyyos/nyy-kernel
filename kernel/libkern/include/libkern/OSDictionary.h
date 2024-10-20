#pragma once

#include <libkern/OSMetaClass.h>
#include <libkern/OSObject.h>
#include <libkern/HashMap.h>
#include <libkern/OSSymbol.h>
#include <string_view>

class OSDictionary : public OSObject {
	OSDeclareDefaultStructors(OSDictionary);

    public:
	static OSDictionary *makeEmpty();
	static OSDictionary *makeWithSize(size_t size);

	virtual bool init() override;
	virtual bool initWithSize(size_t size);
	virtual void free() override;

	virtual OSObject *get(const char *str);
	virtual OSObject *get(OSSymbol *sym);
	virtual bool unset(OSSymbol *sym);

	virtual bool set(std::string_view str, OSMetaClassBase *value);
	virtual bool set(OSSymbol *sym, OSMetaClassBase *value);

    protected:
	HashMap<OSSymbol *, OSMetaClassBase *> *map;
};
