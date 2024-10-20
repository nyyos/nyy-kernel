#pragma once

#include <atomic>
#include <string.h>

class OSMetaClass;
class OSObject;
class OSSymbol;

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

	/* increment object counter */
	void retain();
	/* decrement object counter, if cnt==freeWhen free. */
	void release(int freeWhen = 0);

	virtual bool init() = 0;
	virtual void free() = 0;

	size_t getRefcount() const
	{
		return refcnt;
	}

	// Check if object is kind of class
	bool isKindOf(const OSMetaClass *clazz) const;

    protected:
	std::atomic<size_t> refcnt = 0;
};

class OSMetaClass : public OSMetaClassBase {
	friend class OSMetaClassBase;

    public:
	OSMetaClass(const char *className, const OSMetaClass *superClass,
		    size_t size);

	virtual bool init() override
	{
		return true;
	}
	virtual void free() override
	{
	}

	virtual OSObject *alloc() const = 0;

	const OSMetaClass *getMetaClass() const override
	{
		return const_cast<OSMetaClass *>(this);
	}

	const char *getClassName() const;

	static void initialize();
	static void *preModLoad(const char *ident);
	static bool checkModLoad(void *loadhandle);
	static int postModLoad(void *loadhandle);

    private:
	OSSymbol *className;
	const OSMetaClass *superClass;
	size_t size;
};

template <class T> constexpr T *OSAlloc()
{
	return static_cast<T *>(T::gMetaClass.alloc());
}
