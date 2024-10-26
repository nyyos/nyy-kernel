#include <DevKit/IOPlatformExpert.h>

#define super IOService
OSDefineMetaClassAndStructors(IOPlatformExpert, IOService);

IOPlatformExpert *gIoPlatform;

bool IOPlatformExpert::start(IOService *provider)
{
	if (!super::start(provider))
		return false;
	gIoPlatform = this;
	return true;
}

void IOPlatformExpert::stop(IOService *provider)
{
}
