#pragma once

#include <libkern/OSString.h>

class OSSymbol : public OSString {
	OSDeclareDefaultStructors(OSSymbol);

    public:
	static OSSymbol *fromCStr(const char *cstr);
	static OSSymbol *fromSym(OSSymbol *sym);
	static OSSymbol *fromStr(const OSString *str);

	virtual void initFromCStr(const char *cstr) override;

	virtual void free() override;
};
