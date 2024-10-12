#pragma once

#include <libkern/string.hpp>
#include <string.h>

class OSMetaClass;
class OSObject;

#define OSDeclareCommonStructors(className)                       \
    private:                                                      \
	static const OSMetaClass *const superClass;               \
                                                                  \
    public:                                                       \
	static const OSMetaClass *const metaClass;                \
	static class MetaClass : public OSMetaClass {             \
	    public:                                               \
		MetaClass();                                      \
		virtual OSObject *alloc() const override;         \
	} gMetaClass;                                             \
	friend class className::MetaClass;                        \
	virtual const OSMetaClass *getMetaClass() const override; \
                                                                  \
    protected:                                                    \
	className(const OSMetaClass *);                           \
	virtual ~className() override;

#define OSDeclareDefaultStructors(className) \
	OSDeclareCommonStructors(className); \
                                             \
    protected:                               \
	className();

#define OSDeclareAbstractStructors(className) \
	OSDeclareCommonStructors(className);  \
                                              \
    private:                                  \
	className();                          \
                                              \
    protected:

#define OSDefineMetaClass(className, superName)            \
	className ::MetaClass className ::gMetaClass;      \
	const OSMetaClass *const className::metaClass =    \
		&className ::gMetaClass;                   \
	const OSMetaClass *const className ::superClass =  \
		&superName ::gMetaClass;                   \
	className::className(const OSMetaClass *meta)      \
		: superName(meta)                          \
	{                                                  \
	}                                                  \
	className::~className()                            \
	{                                                  \
	}                                                  \
	const OSMetaClass *className::getMetaClass() const \
	{                                                  \
		return &className::gMetaClass;             \
	}

#define OSMetaClassConstructor(className, superName)             \
	className ::MetaClass::MetaClass()                       \
		: OSMetaClass(#className, className::superClass, \
			      sizeof(className))                 \
	{                                                        \
	}

#define OSDefineDefaultStructors(className, superName) \
	OSObject *className ::MetaClass::alloc() const \
	{                                              \
		return new className;                  \
	}                                              \
	className::className()                         \
		: superName(&gMetaClass)               \
	{                                              \
	}

#define OSDefineAbstractMetaClass(className, superName) \
	OSMetaClassConstructor(className, superName);   \
	OSDefineMetaClass(className, superName);        \
	OSDefineDefaultStructors(className, superName);

#define OSDefineMetaClassAndStructors(className, superName) \
	OSDefineMetaClass(className, superName);            \
	OSDefineDefaultStructors(className, superName);     \
	OSMetaClassConstructor(className, superName);

class OSMetaClassBase {
	friend class OSMetaClass;

    protected:
	virtual ~OSMetaClassBase();

    public:
	virtual const OSMetaClass *getMetaClass() const = 0;

	// Check if object is kind of class
	bool isKindOf(const OSMetaClass *clazz) const;
};

class OSMetaClass : public OSMetaClassBase {
	friend class OSMetaClassBase;

    public:
	OSMetaClass(const char *className, const OSMetaClass *superClass,
		    size_t size)
		: name{ className }
		, superClass{ superClass }
		, size{ size }
	{
	}

	virtual OSObject *alloc() const = 0;

	const OSMetaClass *getMetaClass() const override
	{
		return const_cast<OSMetaClass *>(this);
	}

	const char *getClassName() const;

    private:
	const char *name;
	const OSMetaClass *superClass;
	size_t size;
};
