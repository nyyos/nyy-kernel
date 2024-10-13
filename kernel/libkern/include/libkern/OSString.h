#pragma once

#include <libkern/OSObject.h>
#include <string>

class OSString : public OSObject {
	OSDeclareDefaultStructors(OSString);

    public:
	static OSString *fromCStr(const char *cstr);
	static OSString *fromStr(const OSString *str);

	virtual void initFromCStr(const char *cstr);
	virtual void initFromStr(const OSString *str);

	const char *getCStr() const
	{
		return m_string.c_str();
	}

    protected:
	std::string m_string;
};
