#pragma once

#include <ndk/kmem.h>

template <typename T> [[nodiscard]] constexpr T *kmalloc_type()
{
	return static_cast<T *>(kmalloc(sizeof(T)));
}

template <typename T> constexpr void kfree_type(T *p)
{
	kfree(static_cast<void *>(p), sizeof(T));
}

template <typename T> [[nodiscard]] constexpr T *kcalloc_type(size_t count)
{
	return static_cast<T *>(kcalloc(count, sizeof(T)));
}
