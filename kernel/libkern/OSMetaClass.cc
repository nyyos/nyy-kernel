#include "ndk/mutex.h"
#include <libkern/Memory.h>
#include <libkern/OSDictionary.h>
#include <libkern/mutex.hpp>
#include <libkern/OSMetaClass.h>
#include <ndk/ndk.h>
#include <string.h>

struct StalledData {
	const char *modIdent;
	size_t count;
	size_t capacity;
	OSMetaClass **classes;
};

static constexpr int kClassCapacityIncrement = 40;
enum { kNoDictonaries = 1, kMakingDictonaries, kCompletedBootstrap };

static int sBootstrapState = kNoDictonaries;
static OSSharedPtr<OSDictionary> sAllClassesDict = nullptr;
static StalledData *sStalled;
static mutex_t sAllClassesLock;
static mutex_t sStalledLock;

void OSMetaClass::initialize()
{
	mutex_init(&sStalledLock);
	mutex_init(&sAllClassesLock);
}

void *OSMetaClass::preModLoad(const char *ident)
{
	mutex_acquire(&sStalledLock);

	assert(sStalled == nullptr);
	sStalled = kmalloc_type<StalledData>();

	sStalled->classes =
		kcalloc_type<OSMetaClass *>(kClassCapacityIncrement);
	sStalled->count = 0;
	sStalled->capacity = kClassCapacityIncrement;
	sStalled->modIdent = ident;

	return sStalled;
}

bool OSMetaClass::checkModLoad(void *loadhandle)
{
	return true;
}

int OSMetaClass::postModLoad(void *loadhandle)
{
	assert(sStalled || loadhandle == sStalled);

	switch (sBootstrapState) {
	case kNoDictonaries:
		sBootstrapState = kMakingDictonaries;

		[[clang::fallthrough]];
	case kMakingDictonaries:
		sAllClassesDict =
			OSDictionary::makeWithSize(kClassCapacityIncrement);
		if (!sAllClassesDict) {
			panic("cant create classes dict\n");
		}

		[[clang::fallthrough]];

	case kCompletedBootstrap: {
		size_t i;
		auto modName = const_cast<OSSymbol *>(
			OSSymbol::fromCStr(sStalled->modIdent));
		if (!sStalled->count) {
			// nothing to do
			break;
		}

		mutex_acquire(&sAllClassesLock);
		for (i = 0; i < sStalled->count; i++) {
			const OSMetaClass *me = sStalled->classes[i];
			auto ent = sAllClassesDict->get(
				(const char *)me->className);
			if (ent) {
				printk("in mod '%s': class %s is duplicate",
				       modName->getCStr(),
				       (const char *)me->className);
				break;
			}
		}
		mutex_release(&sAllClassesLock);

		if (i != sStalled->count) {
			break;
		}

		mutex_acquire(&sAllClassesLock);
		for (i = 0; i < sStalled->count; i++) {
			OSMetaClass *me = sStalled->classes[i];

			/* up until now, me->className was a c string */
			me->className =
				OSSymbol::fromCStr((const char *)me->className);

			sAllClassesDict->set(me->className, me);
		}
		mutex_release(&sAllClassesLock);
		sBootstrapState = kCompletedBootstrap;
		break;
	}
	}

	if (sStalled) {
		printk(INFO "module '%s' loaded %ld classes\n",
		       sStalled->modIdent, sStalled->count);
		kfree(sStalled->classes,
		      sizeof(OSMetaClass *) * sStalled->capacity);
		kfree_type(sStalled);
	}

	mutex_release(&sStalledLock);

	return 0;
}

OSMetaClass::OSMetaClass(const char *className, const OSMetaClass *superClass,
			 size_t size)
	: className{ (OSSymbol *)className }
	, superClass{ superClass }
	, size{ size }
{
	if (!sStalled) {
		printk(PANIC "preModLoad() wasnt called for class %s.",
		       (const char *)className);
		panic("runtime internal error");
	} else {
		if (sStalled->count >= sStalled->capacity) {
			auto oldStalled = sStalled->classes;
			auto oldCount = sStalled->capacity;
			auto newCount = oldCount + kClassCapacityIncrement;
			sStalled->classes = static_cast<OSMetaClass **>(
				kcalloc(newCount, sizeof(OSMetaClass *)));
			if (!sStalled->classes) {
				panic("oom?");
			}

			sStalled->capacity = newCount;
			memmove(sStalled->classes, oldStalled,
				sizeof(OSMetaClass *) * oldCount);
			kfree(oldStalled, sizeof(OSMetaClass *) * oldCount);
		}

		sStalled->classes[sStalled->count++] = this;
	}
}
