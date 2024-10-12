#pragma once

#include <libkern/OSObject.h>

class IORegistryEntry : public OSObject {
	virtual bool init() override;

    protected:
	friend class IORegistry;
	IORegistryEntry *next, *parent;
};
