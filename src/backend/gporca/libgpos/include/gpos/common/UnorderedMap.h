//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_CUnorderedMap_H
#define GPOS_CUnorderedMap_H
#include <type_traits>
#include <utility>

#include "gpos/attributes.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"
#include "gpos/memory/unordered_map.h"
#include "gpos/types.h"

namespace gpos
{
template <class K, class T, class Hash = std::hash<K>,
		  class KeyEqual = std::equal_to<>>
class UnorderedMap : public CRefCount
{
	gpos::unordered_map<K, T, Hash, KeyEqual> unordered_map_;

public:
	using iterator = typename decltype(unordered_map_)::iterator;
	using const_iterator = typename decltype(unordered_map_)::const_iterator;

	auto
	begin() const noexcept
	{
		return unordered_map_.begin();
	}
	auto
	end() const noexcept
	{
		return unordered_map_.end();
	}

	// FIXME: the second parameter is retained only for backwards compatibility.
	UnorderedMap(CMemoryPool *mp, ULONG num_chains GPOS_UNUSED = 127)
		: unordered_map_(mp)
	{
	}

	BOOL
	Insert(K key, T t)
	{
		return unordered_map_.emplace(std::move(key), std::move(t)).second;
	}

	// This method is only retained for backwards compatibility.
	std::enable_if_t<std::is_convertible<std::nullptr_t, T>::value, const T &>
	Find(const K &key) const
	{
		auto it = unordered_map_.find(key);
		if (it != unordered_map_.end())
		{
			return it->second;
		}

		// Whoa, doesn't static make for a sinful global var? Why can't you
		// just return nullptr? Well, because the returned reference would
		// otherwise dangle.
		static const T t_from_null = nullptr;
		return t_from_null;
	}

	BOOL
	Replace(const K &key, T t)
	{
		auto it = unordered_map_.find(key);
		if (it == unordered_map_.end())
		{
			return false;
		}
		using std::swap;
		swap(t, it->second);
		return true;
	}

	BOOL
	Delete(const K &key)
	{
		return unordered_map_.erase(key) == 1;
	}

	// FIXME: this is an implicit, narrowing cast from size_t to uint32
	ULONG
	Size() const
	{
		return unordered_map_.size();
	}

	class LegacyIterator
	{
		const_iterator iterator_;
		const const_iterator end_;

	public:
		LegacyIterator(const UnorderedMap *map)
			: iterator_(map->begin()), end_(map->end())
		{
		}

		bool
		Advance()
		{
			if (iterator_ == end_)
			{
				return false;
			}
			++iterator_;
			return true;
		}

		const K &
		Key() const
		{
			return std::prev(iterator_)->first;
		}

		const T &
		Value() const
		{
			return std::prev(iterator_)->second;
		}
	};
};
}  // namespace gpos
#endif	// GPOS_CUnorderedMap_H
