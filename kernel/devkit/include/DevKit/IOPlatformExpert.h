#pragma once

#include "DevKit/IOService.h"

struct IOPlatformExpert : public IOService {
	OSDeclareDefaultStructors(IOPlatformExpert);
#ifndef AMD64
#error "amd64"
#endif

    public:
	virtual bool start(IOService *provider) override;
	virtual void stop(IOService *provider) override;
};
