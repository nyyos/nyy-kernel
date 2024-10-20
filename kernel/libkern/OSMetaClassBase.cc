#include <libkern/OSSymbol.h>
#include <libkern/OSMetaClass.h>

OSMetaClassBase::~OSMetaClassBase()
{
}

bool OSMetaClassBase::isKindOf(const OSMetaClass *clazz) const
{
	const OSMetaClass *current = getMetaClass();
	while (current != nullptr) {
		if (current == clazz) {
			return true;
		}
		current = current->superClass;
	}
	return false;
}

const char *OSMetaClass::getClassName() const
{
	return className->getCStr();
}
