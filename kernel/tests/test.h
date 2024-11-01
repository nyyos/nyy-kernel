#pragma once

enum {
	test_OK,
	test_SKIP,
	test_ERR,
};

struct test_result {
	int status;
	const char *err_msg;
};
