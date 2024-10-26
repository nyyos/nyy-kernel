#pragma once

#include <libkern/OSMetaClass.h>

class OSObject : public OSMetaClassBase {
	OSDeclareAbstractStructors(OSObject);

    public:
	virtual bool init() override;

	/* Keep in mind that:
	 * - Object::free will call delete on the object
	 * - Free may be called before initialization is complete
	 * - Free must never fail
	 */
	virtual void free() override;

	template <class T> T *safe_cast()
	{
		static_assert(std::is_base_of<OSMetaClassBase, T>::value,
			      "T not derived from OSMetaClassBase");
		if (isKindOf(&T::gMetaClass)) {
			return static_cast<T *>(this);
		}
		return nullptr;
	}
};

template <class T> constexpr T *OSDynamicCast(OSObject *object)
{
	if (object == nullptr)
		return nullptr;
	return object->safe_cast<T>();
}
