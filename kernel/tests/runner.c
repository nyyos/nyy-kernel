#include "ndk/ndk.h"
#include "ndk/util.h"
#include "test.h"

typedef struct test_result (*test_function)();

extern struct test_result mutex_test();

struct test_entry {
	const char *test_name;
	test_function fn;
};

static struct test_entry tests[] = { { "mutex acq/rel", mutex_test } };
static struct test_result test_results[elementsof(tests)];
static int passed, skipped, errors;

void test_runner()
{
	for (int i = 0; i < elementsof(tests); i++) {
		printk(INFO "Running test %d/%ld\n", i + 1, elementsof(tests));
		struct test_entry *ent = &tests[i];
		test_results[i] = ent->fn();
		switch (test_results[i].status) {
		case test_OK:
			passed++;
			break;
		case test_SKIP:
			skipped++;
			break;
		case test_ERR:
			printk(WARN "test '%s' failed: %s\n",
			       tests[i].test_name, test_results[i].err_msg);
			errors++;
			break;
		}
	}

	printk(INFO "TEST SUMMARY\n");
	printk(INFO "Passed: %d\n", passed);
	printk(INFO "Skipped: %d\n", skipped);
	printk(INFO "Failed: %d\n", errors);
}
