#pragma once

#include <concepts>
#include <ndk/mutex.h>
#include <utility>

namespace lk
{
struct adopt_lock_t {};
struct defer_lock_t {};

class Mutex {
    public:
	Mutex()
	{
		mutex_init(&_mutex);
	}

	~Mutex()
	{
	}

	Mutex(const Mutex &) = delete;
	Mutex &operator=(const Mutex &) = delete;

	void unlock()
	{
		mutex_release(&_mutex);
	}

	void lock()
	{
		mutex_acquire(&_mutex);
	}

	bool is_locked()
	{
		return _mutex.owner != nullptr;
	}

    private:
	mutex_t _mutex;
};

template <typename T> concept BasicLockable = requires(T a)
{
	{
		a.lock()
	} -> std::same_as<void>;

	{
		a.unlock()

	} -> std::same_as<void>;
};

template <typename T> concept Lockable = BasicLockable<T> && requires(T a)
{
	{
		a.try_lock()
	} -> std::same_as<bool>;
};

template <BasicLockable Mutex> class lock_guard {
    public:
	using mutex_type = Mutex;

	[[nodiscard]] explicit lock_guard(mutex_type &mutex)
		: mutex_(mutex)
	{
		mutex_.lock();
	};

	[[nodiscard]] lock_guard(mutex_type &mutex, adopt_lock_t t)
	{
	}

	// disallow copying
	lock_guard(const lock_guard &) = delete;
	lock_guard &operator=(const lock_guard &) = delete;

	~lock_guard()
	{
		mutex_.unlock();
	}

    private:
	mutex_type &mutex_;
};

template <BasicLockable Mutex> class unique_lock {
    public:
	using mutex_type = Mutex;

	friend void swap(unique_lock &u, unique_lock &v)
	{
		using std::swap;
		swap(u.mutex_, v.mutex_);
		swap(u.is_locked_, v.is_locked_);
	}

	unique_lock()
		: mutex_{ nullptr }
		, is_locked_{ false }
	{
	}

	[[nodiscard]] explicit unique_lock(mutex_type &mutex)
		: mutex_{ &mutex }
		, is_locked_{ false }
	{
		mutex_->lock();
	}

	[[nodiscard]] unique_lock(mutex_type &mutex, adopt_lock_t t)
		: mutex_{ &mutex }
		, is_locked_{ true }
	{
	}

	[[nodiscard]] unique_lock(mutex_type &mutex, defer_lock_t t)
		: mutex_{ &mutex }
		, is_locked_{ false }
	{
	}

	~unique_lock()
	{
		if (is_locked_)
			unlock();
	}

	unique_lock(unique_lock &&other)
		: unique_lock()
	{
		swap(*this, other);
	}

	unique_lock &operator=(unique_lock &&other)
	{
		swap(*this, other);
		return *this;
	}

	unique_lock(const unique_lock &) = delete;
	unique_lock &operator=(const unique_lock &other) = delete;

	void unlock()
	{
		mutex_->unlock();
		is_locked_ = false;
	}

	void lock()
	{
		mutex_->lock();
		is_locked_ = true;
	}

	bool try_lock() requires Lockable<Mutex>
	{
		if (mutex_->try_lock()) {
			is_locked_ = true;
			return true;
		}
		return false;
	}

	mutex_type *release()
	{
		auto mut = mutex_;
		mutex_ = nullptr;
		is_locked_ = false;
		return mut;
	}

	mutex_type *mutex() const
	{
		return mutex_;
	}

	explicit operator bool() const
	{
		owns_lock();
	}

	bool owns_lock() const
	{
		return is_locked_;
	}

    private:
	mutex_type *mutex_;
	bool is_locked_;
};

}
