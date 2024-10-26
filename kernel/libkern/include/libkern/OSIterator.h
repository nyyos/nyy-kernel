#pragma once

#include <libkern/OSObject.h>

class OSIterator : public OSObject {
	OSDeclareAbstractStructors(OSIterator);

    public:
	virtual void reset() = 0;

	virtual bool isValid() = 0;

	virtual OSObject *getNextObject() = 0;
};
