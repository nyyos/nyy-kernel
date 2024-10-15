#include <libkern/OSArray.h>
#include <ndk/ndk.h>
#include <ndk/kmem.h>

#define super OSObject

OSDefineMetaClassAndStructors(OSArray, OSObject);

OSArray *OSArray::makeEmpty()
{
	auto arr = OSAlloc<OSArray>();
	if (!arr->init()) {
		arr->free();
		return nullptr;
	}
	return arr;
}

bool OSArray::init()
{
	if (!super::init())
		return false;

	m_array = nullptr;
	m_size = 0;
	m_capacity = 0;

	return true;
}

void OSArray::free()
{
	if (m_array) {
		for (size_t i = 0; i < m_size; i++) {
			m_array[i]->release();
			m_array[i] = nullptr;
		}
		kfree(m_array, m_capacity * sizeof(OSObject *));
		m_array = nullptr;
		m_capacity = 0;
		m_size = 0;
	}
}

void OSArray::resize(size_t size)
{
	if (size == 0)
		size = 4;
	auto newArray = (OSObject **)kmalloc(size * sizeof(OSObject *));
	for (size_t i = 0; i < m_size; i++) {
		newArray[i] = std::move(m_array[i]);
	}
	m_array = newArray;
	m_capacity = size;
}

void OSArray::insert(OSObject *obj)
{
	if ((m_size + 1) >= m_capacity)
		resize(m_capacity * 2);

	obj->retain();
	m_array[m_size++] = obj;
}

OSObject *OSArray::pop()
{
	return m_array[--m_size];
}

size_t OSArray::getCount() const
{
	return m_size;
}

OSObject *OSArray::getEntry(size_t index)
{
	if (index > m_size)
		return nullptr;
	auto obj = m_array[index];
	obj->retain();
	return obj;
}
