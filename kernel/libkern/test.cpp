#include <memory>
#include <ndk/ndk.h>
#include <lk/mutex.hpp>

struct Foo {};

void test_fn_cpp()
{
	auto allocator = std::allocator<Foo>();
	(void)allocator.allocate(32);

	auto mut = lk::Mutex();
	lk::unique_lock<lk::Mutex> uniq;
	{
		const auto guard = lk::lock_guard(mut);
		uniq = lk::unique_lock(mut, lk::adopt_lock_t());
		uniq.release();
	}
}

extern "C" {
void test_fn()
{
	test_fn_cpp();
}
}
