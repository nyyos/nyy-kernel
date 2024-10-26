#pragma once

#include "libkern/OSArray.h"
#include <libkern/OSSymbol.h>
#include <libkern/OSDictionary.h>
#include <libkern/OSMetaClass.h>
#include <libkern/OSObject.h>

class IORegistryPlane;

class IORegistryEntry : public OSObject {
	OSDeclareDefaultStructors(IORegistryEntry);

    public:
	static IORegistryEntry *fromPath(const char *path,
					 const IORegistryPlane *plane = nullptr,
					 IORegistryEntry *fromEntry = nullptr);

	/**
	 * @brief initialize the registry, create root
	 *
	 * @return registry root 
	 */
	static IORegistryEntry *InitRegistry();

	virtual void setName(const char *str);

	virtual void setName(OSSymbol *to);

	[[nodiscard]] virtual OSSymbol *getName();

	virtual bool init(OSDictionary *dictionary = nullptr);

	virtual void free() override;

	virtual bool attachToParent(IORegistryEntry *parent,
				    const IORegistryPlane *plane);

	virtual bool attachToChild(IORegistryEntry *child,
				   const IORegistryPlane *plane);

	virtual bool setProperty(OSSymbol *key, OSObject *value);
	virtual bool setProperty(const char *key, OSObject *value);

    protected:
	IORegistryEntry *m_next;
	IORegistryEntry *m_parent;

	OSSymbol *m_name;

	OSDictionary *m_properties;

	OSArray *m_links;

    private:
	using OSObject::init;

	int m_registryID;
};
