#include <ndk/ndk.h>
#include <ndk/mutex.h>
#include <ndk/sched.h>

void mutex_init(mutex_t *mutex)
{
	SPINLOCK_INIT(&mutex->queue_lock);
	TAILQ_INIT(&mutex->wait_queue);
	mutex->owner = nullptr;
	mutex->flag = 0;
}

void mutex_acquire(mutex_t *mutex)
{
	assert(mutex);
	thread_t *thread = curthread();
	if (mutex->owner != nullptr && mutex->owner == thread) {
		assert(!"mutex recursive acquire");
	}

retry:
	irql_t irql = irql_raise(IRQL_DISPATCH);
	uint8_t unlocked = 0;
	for (int i = 0; i < 10000; i++) {
		if (__atomic_compare_exchange_n(&mutex->flag, &unlocked, 1, 0,
						__ATOMIC_ACQUIRE,
						__ATOMIC_RELAXED)) {
			mutex->owner = thread;
			irql_lower(irql);
			return;
		}
		unlocked = 0;
		port_spin_hint();
	}

	spinlock_acquire(&mutex->queue_lock);
	TAILQ_INSERT_TAIL(&mutex->wait_queue, thread, entry);
	spinlock_release(&mutex->queue_lock);

	irql_lower(irql);

	sched_yield();

	goto retry;
}

void mutex_release(mutex_t *mutex)
{
	assert(mutex->owner == curthread());
	mutex->owner = nullptr;

	spinlock_acquire(&mutex->queue_lock);
	thread_t *tp = TAILQ_FIRST(&mutex->wait_queue);
	if (tp != nullptr) {
		TAILQ_REMOVE(&mutex->wait_queue, tp, entry);
	}
	spinlock_release(&mutex->queue_lock);

	__atomic_store_n(&mutex->flag, 0, __ATOMIC_RELEASE);

	if (tp != nullptr)
		sched_resume(tp);
}
