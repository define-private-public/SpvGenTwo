#pragma once

#include "HashMapIterator.h"

namespace spvgentwo
{
	template <class Key, class Value>
	// with chaining => multimap
	class HashMap
	{
	public:
		static constexpr auto DefaultBucktCount = 64u;

		using Node = NodeT<Key, Value>;
		using Bucket = List<Node>;

		using Iterator = HashMapIterator<Key, Value>;
		using ValueType = Node;
		using ReferenceType = Node&;
		using PointerType = Node*;

		struct Range
		{
			typename Bucket::Iterator m_Begin;
			typename Bucket::Iterator m_End;

			typename Bucket::Iterator begin() const { return m_Begin; }
			typename Bucket::Iterator end() const { return m_End; }
		};

	public:

		HashMap(IAllocator* _pAllocator = nullptr, const unsigned int _buckets = DefaultBucktCount);
		HashMap(HashMap&& _other) noexcept;

		~HashMap();

		HashMap& operator=(HashMap&& _other) noexcept;

		template <class ... Args>
		Node& emplace(Args&& ... _args);

		// returns existing node if duplicate
		template <class ... Args>
		Node& emplaceUnique(Args&& ... _args);

		template <class ... Args>
		Node& emplaceUnique(const Key& _key, Args&& ... _args);

		Node& newNodeUnique(const Hash64& _hash);

		Value* get(const Hash64 _hash) const;

		// only enable overload of Key type differs from Hash64
		template <class T = Key, typename = stdrep::enable_if_t<stdrep::is_same_v<T, Key> && !stdrep::is_same_v<T, Hash64>>>
		Value* get(const T& _key) const { return get(hash(_key)); }

		Range getRange(const Hash64 _hash) const;

		// only enable overload of Key type differs from Hash64
		template <class T = Key, typename = stdrep::enable_if_t<stdrep::is_same_v<T, Key> && !stdrep::is_same_v<T, Hash64>>>
		Range getRange(const T& _key) const { return getRange(hash(_key)); }

		Iterator find(const Key& _key) const;

		Key* findKey(const Value& _value) const;

		// remove value pointed to by pos, returns the next elements iterator or end
		Iterator erase(Iterator pos);

		// remove all elements with Key _key
		unsigned int eraseRange(const Key& _key);

		unsigned int count(const Hash64 _hash) const;
		unsigned int count(const Key& _key) const { return count(hash(_key)); }

		const Bucket& getBucket(const unsigned int _index) const { return m_pBuckets[_index]; }
		unsigned int getBucketCount() const { return m_Buckets; }

		Iterator begin() const;
		Iterator end() const { return Iterator(m_pBuckets + m_Buckets, m_pBuckets + m_Buckets, nullptr); }

		void clear();

		unsigned int elements() const { return m_Elements; }

	private:
		void destroy();

	private:
		IAllocator* m_pAllocator = nullptr;
		Bucket* m_pBuckets = nullptr;
		unsigned int m_Buckets = 0u;
		unsigned int m_Elements = 0u;
	};

	template<class Key, class Value>
	inline HashMap<Key, Value>::HashMap(IAllocator* _pAllocator, const unsigned int _buckets) :
		m_pAllocator(_pAllocator), m_Buckets(_buckets)
	{
		m_pBuckets = reinterpret_cast<Bucket*>(m_pAllocator->allocate(m_Buckets * sizeof(Bucket)));
		for (auto i = 0u; i < m_Buckets; ++i)
		{
			new(m_pBuckets + i) Bucket(m_pAllocator);
		}
	}

	template<class Key, class Value>
	inline HashMap<Key, Value>::HashMap(HashMap&& _other) noexcept :
		m_pAllocator(_other.m_pAllocator),
		m_pBuckets(_other.m_pBuckets),
		m_Buckets(_other.m_Buckets),
		m_Elements(_other.m_Elements)
	{
		_other.m_pAllocator = nullptr;
		_other.m_pBuckets = 0u;
		_other.m_Buckets = 0u;
		_other.m_Elements = 0u;
	}

	template<class Key, class Value>
	inline void HashMap<Key, Value>::destroy()
	{
		if (m_pBuckets != nullptr && m_pAllocator != nullptr)
		{
			for (auto i = 0u; i < m_Buckets; ++i)
			{
				m_pBuckets[i].~List();
			}
			m_pAllocator->deallocate(m_pBuckets, m_Buckets * sizeof(Bucket));
			m_pBuckets = nullptr;
			m_pAllocator = nullptr;
			m_Elements = 0u;
		}
	}

	template<class Key, class Value>
	inline void HashMap<Key, Value>::clear()
	{
		if (m_pBuckets != nullptr)
		{
			for (auto i = 0u; i < m_Buckets; ++i)
			{
				m_pBuckets[i].clear();
			}
		}
		m_Elements = 0u;
	}

	template<class Key, class Value>
	inline HashMap<Key, Value>::~HashMap()
	{
		destroy();
	}

	template<class Key, class Value>
	inline HashMap<Key, Value>& HashMap<Key, Value>::operator=(HashMap&& _other) noexcept
	{
		if (this != &_other) return *this;

		// free left side
		destroy();

		m_Buckets = _other.m_Buckets;
		m_pAllocator = _other.m_pAllocator;
		m_pBuckets = _other.m_pBuckets;
		m_Elements = _other.m_Elements;

		_other.m_Buckets = 0u;
		_other.m_pAllocator = nullptr;
		_other.m_pBuckets = nullptr;
		_other.m_Elements = 0u;

		return *this;
	}

	template<class Key, class Value>
	inline Value* HashMap<Key, Value>::get(const Hash64 _hash) const
	{
		const auto index = _hash % m_Buckets;

		for (Node& n : m_pBuckets[index])
		{
			if (n.hash == _hash)
				return &n.kv.value;
		}

		return nullptr;
	}

	template<class Key, class Value>
	inline typename HashMap<Key, Value>::Range HashMap<Key, Value>::getRange(const Hash64 _hash) const
	{
		const auto index = _hash % m_Buckets;
		const Bucket& bucket = m_pBuckets[index];

		auto first = bucket.end();
		auto last = first;

		for (auto it = bucket.begin(); it != bucket.end(); ++it)
		{
			const Node& n = *it;
			if (n.hash == _hash)
			{
				if (first == bucket.end())
				{
					first = it;
				}
				last = it + 1u;
			}
			
		}

		return { first , last};
	}

	template<class Key, class Value>
	inline typename HashMap<Key, Value>::Iterator HashMap<Key, Value>::erase(Iterator pos)
	{
		if(pos && pos != end())
		{
			pos.m_pBucket->erase(pos.m_element);
			return ++pos;
		}
		return end();
	}

	template<class Key, class Value>
	inline unsigned int HashMap<Key, Value>::eraseRange(const Key& _key)
	{
		Hash64 h = 0u;

		if constexpr (stdrep::is_same_v<Key, Hash64>)
		{
			h = _key;
		}
		else
		{
			h = hash(_key);
		}

		unsigned int keys = 0u;
		const auto index = h % m_Buckets;

		Bucket& bucket = m_pBuckets[index];

		for (auto it = bucket.begin(), e = bucket.end(); it != e;)
		{
			if (it->hash == h)
			{
				it = bucket.erase(it);
				++keys;
			}
			else 
			{
				++it;
			}
		}

		return keys;
	}

	template<class Key, class Value>
	inline unsigned int HashMap<Key, Value>::count(const Hash64 _hash) const
	{
		unsigned int keys = 0u;
		const auto index = _hash % m_Buckets;

		for (const Node& n : m_pBuckets[index])
		{
			if (n.hash == _hash)
			{
				++keys;
			}
		}

		return keys;
	}

	template<class Key, class Value>
	inline typename HashMap<Key, Value>::Iterator HashMap<Key, Value>::find(const Key& _key) const
	{
		Hash64 h = 0u;

		if constexpr (stdrep::is_same_v<Key, Hash64>)
		{
			h = _key;
		}
		else
		{
			h = hash(_key);
		}

		const auto index = h % m_Buckets;

		const Bucket& bucket = m_pBuckets[index];

		for (auto it = bucket.begin(), e = bucket.end(); it != e; ++it)
		{
			if (it->hash == h)
			{
				return Iterator(m_pBuckets + index, m_pBuckets + m_Buckets, it);
			}
		}

		return end();
	}

	template<class Key, class Value>
	inline Key* HashMap<Key, Value>::findKey(const Value& _value) const
	{
		for (auto i = 0u; i < m_Buckets; ++i)
		{
			for (Bucket& b : m_pBuckets[i])
			{
				for (Node& n : b)
				{
					if (n.kv.value == _value)
					{
						return &n.kv.key;
					}
				}
			}
		}

		return nullptr;
	}

	template<class Key, class Value>
	template<class ...Args>
	inline typename HashMap<Key, Value>::Node& HashMap<Key, Value>::emplace(Args&& ..._args)
	{
		Entry<Node>* pNode = Entry<Node>::create(m_pAllocator, stdrep::forward<Args>(_args)...);

		Node& n = pNode->inner();

		if constexpr (stdrep::is_same_v<Key, Hash64>)
		{
			n.hash = n.kv.key;
		}
		else
		{
			n.hash = hash(n.kv.key);
		}

		const auto index = n.hash % m_Buckets;

		// append to chain
		m_pBuckets[index].append_entry(pNode);

		++m_Elements;

		return n;
	}

	template<class Key, class Value>
	template<class ...Args> // (non trivial) key is constructed from args and then used to compute the hash
	inline typename HashMap<Key, Value>::Node& HashMap<Key, Value>::emplaceUnique(Args&& ..._args)
	{
		Entry<Node>* pNode = Entry<Node>::create(m_pAllocator, stdrep::forward<Args>(_args)...);

		Node& n = pNode->inner();

		if constexpr (stdrep::is_same_v<Key, Hash64>)
		{
			n.hash = n.kv.key;
		}
		else
		{
			n.hash = hash(n.kv.key);
		}

		const auto index = n.hash % m_Buckets;

		for (Node& existing : m_pBuckets[index])
		{
			if (existing.hash == n.hash)
			{
				m_pAllocator->destruct(pNode);
				return existing;
			}
		}

		m_pBuckets[index].append_entry(pNode);

		++m_Elements;

		return n;
	}

	template<class Key, class Value>
	template<class ...Args>
	inline typename HashMap<Key, Value>::Node& HashMap<Key, Value>::emplaceUnique(const Key& _key, Args&& ..._args)
	{
		Hash64 h = 0u;

		if constexpr (stdrep::is_same_v<Key, Hash64>)
		{
			h = _key;
		}
		else
		{
			h = hash(_key);
		}

		const auto index = h % m_Buckets;

		for (Node& existing : m_pBuckets[index])
		{
			if (existing.hash == h)
			{
				return existing;
			}
		}

		Node& n = m_pBuckets[index].emplace_back(_key, stdrep::forward<Args>(_args)...);
		n.hash = h;

		++m_Elements;

		return n;
	}

	template<class Key, class Value>
	inline typename HashMap<Key, Value>::Node& HashMap<Key, Value>::newNodeUnique(const Hash64& _hash)
	{
		const auto index = _hash % m_Buckets;

		for (Node& existing : m_pBuckets[index])
		{
			if (existing.hash == _hash)
			{
				return existing;
			}
		}

		Node& n = m_pBuckets[index].emplace_back();
		n.hash = _hash;

		++m_Elements;

		return n;
	}

	template<class Key, class Value>
	inline typename HashMap<Key, Value>::Iterator HashMap<Key, Value>::begin() const
	{
		for (unsigned int i = 0u; i < m_Buckets; ++i)
		{
			if (m_pBuckets[i].empty() == false)
			{
				return Iterator(m_pBuckets + i, m_pBuckets + m_Buckets, m_pBuckets[i].begin());
			}
		}

		return end();
	}
} // !spvgentwo