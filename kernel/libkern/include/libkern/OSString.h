#pragma once

#include <libkern/OSObject.h>
#include <string>

class OSString : public OSObject {
	OSDeclareDefaultStructors(OSString);

    public:
	static OSString *fromCStr(const char *cstr);
	static OSString *fromStr(const OSString *str);

	void initFromCStr(const char *cstr);
	void initFromStr(const OSString *str);

	const char *getCStr() const
	{
		return m_string.c_str();
	}

    private:
	std::string m_string;
};
