#pragma once

#include <libkern/OSMetaClass.h>
#include <atomic>

class OSObject : public OSMetaClassBase {
	OSDeclareAbstractStructors(OSObject);

    public:
	/* increment object counter */
	void retain();
	/* decrement object counter, if cnt==freeWhen free. */
	void release(int freeWhen = 0);

	virtual bool init();

	/* Keep in mind that:
	 * - Object::free will call delete on the object
	 * - Free may be called before initialization is complete
	 * - Free must never fail
	 */
	virtual void free();

	template <class T> T *safe_cast()
	{
		static_assert(std::is_base_of<OSMetaClassBase, T>::value,
			      "T not derived from OSMetaClassBase");
		if (isKindOf(&T::gMetaClass)) {
			return static_cast<T *>(this);
		}
		return nullptr;
	}

    private:
	std::atomic<std::size_t> refcnt;
};
