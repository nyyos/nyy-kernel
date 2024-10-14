#pragma once

#include <libkern/OSMetaClass.h>
#include <libkern/OSObject.h>
#include <libkern/HashMap.h>
#include <libkern/OSSymbol.h>
#include <string_view>

class OSDictionary : public OSObject {
	OSDeclareDefaultStructors(OSDictionary);

    public:
	virtual bool init() override;
	virtual void free() override;

	// symbol reference now owned by dictionary
	virtual void set(OSSymbol *sym, OSObject *value);
	virtual OSObject *get(OSSymbol *sym);
	virtual bool unset(OSSymbol *sym);

	virtual void set(std::string_view str, OSObject *value);

    protected:
	HashMap<OSSymbol *, OSObject *> *map;
};
