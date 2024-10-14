#pragma once

#include "ndk/ndk.h"
#include <functional>
#include <cstddef>
#include <optional>

template <typename K, typename V, typename Compare = std::equal_to<K>,
	  typename Hash = std::hash<K> >
class HashMap {
    private:
	struct MapEntry final {
		MapEntry()
			: hash{ 0 }
			, valid{ false }
		{
		}

		MapEntry(K k, Hash &hashfn)
			: key{ k }
			, hash{ hashfn(k) }
			, valid{ false }
		{
		}

		K key;
		V value;
		size_t hash;
		bool valid;
	};

    public:
	static constexpr const int DEFAULT_CAPACITY = 16;

	HashMap(size_t initial, Compare compare = Compare(), Hash hash = Hash())
		: m_capacity{ initial }
		, m_count{ 0 }
		, m_entries{ new MapEntry[initial]() }
		, compare{ compare }
		, hashfn{ hash }
	{
	}

	HashMap()
		: HashMap(DEFAULT_CAPACITY)
	{
	}

	~HashMap()
	{
		delete[] m_entries;
		m_entries = nullptr;
	}

	HashMap(const HashMap &) = delete;
	HashMap &operator=(const HashMap &other) = delete;
	HashMap(HashMap &&other) = delete;
	HashMap &operator=(HashMap &&other) = delete;

	bool insert(K key, V val)
	{
		if (m_count + 1 > (m_capacity * 60 / 100)) {
			// if more than 0.6 of capacity, increase by 1.5
			auto cap = m_capacity + m_capacity / 2;
			doResize(cap);
		}

		auto hint = MapEntry(key, hashfn);
		auto entry = findEntry(m_entries, m_capacity, hint);

		if (entry->valid)
			return false;

		if (entry->hash != 0xDEAD)
			m_count++;

		entry->valid = true;
		entry->hash = hint.hash;
		entry->key = K(key);
		entry->value = V(val);

		return true;
	}

	bool resize(size_t size)
	{
		if (size == 0)
			return false;
		if (size < m_count)
			return false;

		doResize(size);
		return true;
	}

	std::optional<V> update(K key, V val)
	{
		auto hint = MapEntry(key, hashfn);
		auto entry = findEntry(m_entries, m_capacity, hint);
		if (entry->valid) {
			std::swap(entry->value, val);
			entry->value = val;
			return val;
		}
		insert(key, val);
		return {};
	}

	bool remove(K key)
	{
		if (m_count == 0)
			return false;

		auto hint = MapEntry(key, hashfn);
		auto entry = findEntry(m_entries, m_capacity, hint);
		if (entry->valid == false)
			return false;

		// place tombstone
		entry->valid = false;
		entry->hash = 0xDEAD;

		if (m_count < (m_capacity / 4)) {
			doResize(m_capacity / 2);
		}

		return true;
	}

	std::optional<V> lookup(K key)
	{
		if (m_count == 0)
			return {};

		auto hint = MapEntry(key, hashfn);
		auto entry = findEntry(m_entries, m_capacity, hint);
		if (entry->valid == false)
			return {};

		return entry->value;
	}

	class iterator {
	    public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = std::pair<K, V>;
		using pointer = value_type *;
		using reference = value_type &;

		iterator(MapEntry *ptr, MapEntry *end)
			: m_ptr{ ptr }
			, m_end{ end }
		{
			advanceToValid();
		}

		value_type operator*() const
		{
			return { m_ptr->key, m_ptr->value };
		}

		pointer operator->() const
		{
			return &(**this);
		}

		iterator &operator++()
		{
			++m_ptr;
			advanceToValid();
			return *this;
		}

		iterator operator++(int)
		{
			iterator tmp = *this;
			++(*this);
			return tmp;
		}

		bool operator==(const iterator &other) const
		{
			return m_ptr == other.m_ptr;
		}

		bool operator!=(const iterator &other) const
		{
			return !(*this == other);
		}

	    private:
		MapEntry *m_ptr;
		MapEntry *m_end;

		void advanceToValid()
		{
			while (m_ptr != m_end && !m_ptr->valid) {
				++m_ptr;
			}
		}
	};

	iterator begin()
	{
		return iterator(m_entries, m_entries + m_capacity);
	}

	iterator end()
	{
		return iterator(m_entries + m_capacity, m_entries + m_capacity);
	}

    private:
	MapEntry *findEntry(MapEntry *entries, size_t capacity, MapEntry &hint)
	{
		size_t index = hint.hash % capacity;
		for (;;) {
			auto entry = &entries[index];
			MapEntry *tombstone = nullptr;
			if (entry->valid == true && entry->hash == hint.hash) {
				if (compare(entry->key, hint.key))
					return entry;
			} else if (entry->valid == false) {
				if (entry->hash == 0xDEAD) {
					// found a tombstone
					if (tombstone == nullptr)
						tombstone = entry;
				} else {
					return tombstone != nullptr ?
						       tombstone :
						       entry;
				}
			}

			index = (index + 1) % capacity;
		}
	}

	void doResize(size_t capacity)
	{
		if (capacity <= 1)
			capacity = 4;
		auto entries = new MapEntry[capacity]();

		m_count = 0;
		for (int i = 0; i < m_capacity; i++) {
			auto &entry = m_entries[i];
			if (entry.valid == false)
				continue;

			auto dest = findEntry(entries, capacity, entry);
			dest->key = std::move(entry.key);
			dest->value = std::move(entry.value);
			dest->valid = true;
			dest->hash = std::move(entry.hash);
			m_count++;
		}

		delete[] m_entries;
		m_entries = entries;
		m_capacity = capacity;
	}

	size_t m_capacity;
	size_t m_count;
	MapEntry *m_entries;

	Compare compare;
	Hash hashfn;
};
