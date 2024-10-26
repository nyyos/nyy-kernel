#pragma once

#include "libkern/OSMetaClass.h"
#include <concepts>

struct no_retain_t {};

template <class T> class OSSharedPtr {
    public:
	OSSharedPtr()
		: m_ptr{ nullptr }
	{
	}

	OSSharedPtr(T *ptr = nullptr)
		: m_ptr{ ptr }
	{
		if (m_ptr)
			m_ptr->retain();
	}

	OSSharedPtr(T *ptr, no_retain_t)
		: m_ptr{ ptr }
	{
	}

	~OSSharedPtr()
	{
		release();
	}

	// copy: increment refcnt
	OSSharedPtr(const OSSharedPtr<T> &other)
	{
		m_ptr = other.m_ptr;
		if (m_ptr != nullptr)
			m_ptr->retain();
	}

	// move: transfer ownership
	OSSharedPtr(OSSharedPtr<T> &&other)
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
	}

	OSSharedPtr<T> &operator=(OSSharedPtr<T> &&other)
	{
		if (this != &other) {
			release();
			m_ptr = other.m_ptr;
			m_ptr->retain();
			other.release();
		}
		return *this;
	}

	T *get() const
	{
		return static_cast<T *>(m_ptr);
	}

	T *operator->() const
	{
		return get();
	}

	T *operator*() const
	{
		return get();
	}

	OSSharedPtr<T> &operator=(const OSSharedPtr<T> &other)
	{
		if (this != &other) {
			release();
			m_ptr = other.m_ptr;
			m_ptr->retain();
		}
		return *this;
	}

	explicit operator bool() const
	{
		return m_ptr;
	}

	void release()
	{
		if (m_ptr) {
			m_ptr->release();
			m_ptr = nullptr;
		}
	}

    public:
	OSMetaClassBase *m_ptr;
};

template <class T> OSSharedPtr<T> OSMakeShared()
{
	return OSSharedPtr(OSAlloc<T>(), no_retain_t{});
}

template <class T> OSSharedPtr<T> OSMakeShared(T *raw)
{
	return OSSharedPtr(raw, no_retain_t{});
}
