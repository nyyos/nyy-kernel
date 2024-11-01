#include "ndk/ndk.h"
#include "ndk/util.h"
#include "test.h"

typedef struct test_result (*test_function)();

extern struct test_result mutex_test();

test_function tests[] = { mutex_test };

void test_runner()
{
	for (int i = 0; i < elementsof(tests); i++) {
		printk(INFO "Running test %d/%ld\n", i + 1, elementsof(tests));
		tests[i]();
	}
}
