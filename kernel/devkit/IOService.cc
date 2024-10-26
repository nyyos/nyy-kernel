#include <DevKit/IOService.h>

#define super IORegistryEntry
OSDefineMetaClassAndStructors(IOService, IORegistryEntry);

bool IOService::start(IOService *provider)
{
	return true;
}

void IOService::stop(IOService *provider)
{
}

bool IOService::init(OSDictionary *properties)
{
	if (!super::init(properties))
		return false;

	return true;
}

void IOService::free()
{
	super::free();
}
