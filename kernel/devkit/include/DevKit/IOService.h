#pragma once

#include <libkern/OSDictionary.h>
#include <DevKit/IORegistryEntry.h>

class IOService : public IORegistryEntry {
	OSDeclareDefaultStructors(IOService);

	virtual bool start(IOService *provider);
	virtual void stop(IOService *provider);
	virtual bool init(OSDictionary *properties) override;
	virtual void free() override;
};
