#include <DevKit/Common.h>
#include <libkern/OSSymbol.h>
#include <libkern/OSDictionary.h>
#include <DevKit/IORegistryEntry.h>

class IORegistryPlane : public IORegistryEntry {
	OSDeclareDefaultStructors(IORegistryPlane);

    public:
	virtual bool init(OSDictionary *properties) override;
};

#define super IORegistryEntry
OSDefineMetaClassAndStructors(IORegistryPlane, IORegistryEntry);

bool IORegistryPlane::init(OSDictionary *properties)
{
	if (!super::init(properties))
		return false;

	return true;
}

#undef super
#define super OSObject
OSDefineMetaClassAndStructors(IORegistryEntry, OSObject);

IORegistryEntry *gIoRootEntry = nullptr;
OSSharedPtr<OSDictionary> gIoPlanes = nullptr;
std::atomic<long> gIoLastId = 0;

bool IORegistryEntry::init(OSDictionary *properties)
{
	if (!super::init())
		return false;

	m_next = nullptr;
	m_parent = nullptr;

	if (properties == nullptr) {
		m_properties = OSAlloc<OSDictionary>();
		if (!m_properties || !m_properties->init())
			return false;
	} else {
		properties->retain();
		m_properties = properties;
	}

	return true;
}

void IORegistryEntry::free()
{
	m_properties->release();

	super::free();
}

IORegistryEntry *IORegistryEntry::fromPath(const char *path,
					   const IORegistryPlane *plane,
					   IORegistryEntry *fromEntry)
{
	return nullptr;
}

IORegistryEntry *IORegistryEntry::InitRegistry()
{
	assert(!gIoRootEntry);
	gIoRootEntry = OSAlloc<IORegistryEntry>();
	gIoPlanes = OSDictionary::makeEmpty();
	assert(gIoRootEntry && gIoPlanes);

	gIoRootEntry->init(nullptr);

	gIoRootEntry->m_registryID = ++gIoLastId;
	gIoRootEntry->setName("Root");
	gIoRootEntry->setProperty(kIoRegistryPlanesKey, *gIoPlanes);

	printk(INFO "created root DevKit IORegistryEntry\n");

	return gIoRootEntry;
}

bool IORegistryEntry::setProperty(OSSymbol *key, OSObject *obj)
{
	assert(key && obj && m_properties);
	m_properties->set(key, obj);
	return true;
}

bool IORegistryEntry::setProperty(const char *key, OSObject *obj)
{
	OSSymbol *keySym = OSSymbol::fromCStr(key);
	auto ret = setProperty(keySym, obj);
	keySym->release();
	return ret;
}

void IORegistryEntry::setName(const char *str)
{
	if (m_name != nullptr)
		m_name->release();
	m_name = OSSymbol::fromCStr(str);
}

void IORegistryEntry::setName(OSSymbol *to)
{
	if (m_name != nullptr)
		m_name->release();
	to->retain();
	m_name = to;
}
OSSymbol *IORegistryEntry::getName()
{
	return OSSymbol::fromSym(m_name);
}

bool IORegistryEntry::attachToChild(IORegistryEntry *child,
				    const IORegistryPlane *plane)
{
	if (this == child)
		return false;

	child->attachToParent(this, plane);

	return true;
}

bool IORegistryEntry::attachToParent(IORegistryEntry *parent,
				     const IORegistryPlane *plane)
{
	if (this == parent)
		return false;

	parent->attachToChild(this, plane);

	return true;
}
