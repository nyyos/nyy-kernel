#include <memory>
#include <ndk/ndk.h>

struct Foo {};

void test_fn_cpp()
{
	auto allocator = std::allocator<Foo>();
	(void)allocator.allocate(32);
}

extern "C" {
void test_fn()
{
	test_fn_cpp();
}
}
