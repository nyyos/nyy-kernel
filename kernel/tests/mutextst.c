#include <ndk/mutex.h>
#include <ndk/sched.h>
#include <ndk/time.h>

#include "test.h"

static void t_mutex(void *ctx1, void *)
{
	mutex_t *mtx = ctx1;
	timer_t timer;
	timer_init(&timer);

	sched_wait_single(mtx, 100);

	timer_set_in(&timer, MS2NS(500), nullptr);
	timer_install(&timer, nullptr, nullptr);

	sched_wait_single(&timer, kWaitTimeoutInfinite);

	mutex_release(mtx);

	sched_exit_destroy();
}

struct test_result mutex_test()
{
	mutex_t mtx;
	mutex_init(&mtx);

	thread_t t1;
	sched_init_thread(&t1, t_mutex, kPriorityHighMax, &mtx, nullptr);
	sched_resume(&t1);

	sched_wait_single(&mtx, 5000);

	return (struct test_result){ test_OK };
}
