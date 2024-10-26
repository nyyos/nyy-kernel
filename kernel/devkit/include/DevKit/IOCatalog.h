#pragma once

#include "libkern/OSDictionary.h"
#include "libkern/OSObject.h"

class IOCatalog : public OSObject {
	OSDeclareDefaultStructors(IOCatalog);

    public:
	static IOCatalog *Initialize();

	virtual bool init() override;
	virtual void free() override;

	void addPersonality(OSDictionary *dict);

	void printPersonalities() const;

    private:
	OSSharedPtr<OSDictionary> personalities = nullptr;
};
