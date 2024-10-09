#pragma once

#include <atomic>

class Object {
    protected:
	Object();
	constexpr virtual ~Object() = default;

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

    private:
	std::atomic<std::size_t> refcnt;
};
