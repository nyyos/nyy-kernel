#pragma once

#include "libkern/OSSharedPtr.h"
#include <cstddef>
#include <libkern/OSObject.h>

class OSArray : public OSObject {
	OSDeclareDefaultStructors(OSArray);

    public:
	static OSSharedPtr<OSArray> makeEmpty();
	static OSSharedPtr<OSArray> fromArray(OSArray *array);

	virtual bool init() override;
	virtual void free() override;

	virtual void resize(size_t size);

	virtual size_t getCount() const;
	virtual OSObject *getEntry(size_t index);

	virtual void insert(OSObject *obj);
	virtual void insert(const char *cstr);
	virtual OSObject *pop();

    protected:
	OSObject **m_array;
	size_t m_capacity;
	size_t m_size;
};
