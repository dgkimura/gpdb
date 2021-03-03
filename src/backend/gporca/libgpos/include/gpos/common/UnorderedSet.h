//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_UnorderedSet_H
#define GPOS_UnorderedSet_H

#include "gpos/common/CRefCount.h"
#include "gpos/memory/unordered_set.h"

namespace gpos
{
template <class Key, class Hash = std::hash<Key>,
		  class KeyEqual = std::equal_to<>>
class UnorderedSet : public CRefCount
{
	using Set = gpos::unordered_set<Key, Hash, KeyEqual>;
	Set unordered_set_;

public:
	using iterator = typename Set::iterator;
	using const_iterator = typename Set::const_iterator;

	auto
	begin() const noexcept
	{
		return unordered_set_.begin();
	}
	auto
	end() const noexcept
	{
		return unordered_set_.end();
	}

	UnorderedSet(CMemoryPool *mp, ULONG size GPOS_UNUSED = 127)
		: unordered_set_(mp)
	{
	}

	BOOL
	Contains(const Key &key) const noexcept
	{
		return unordered_set_.find(key) != unordered_set_.end();
	}

	BOOL
	Insert(Key key)
	{
		return unordered_set_.emplace(std::move(key)).second;
	}

	ULONG
	Size() const noexcept
	{
		return unordered_set_.size();
	}

	class LegacyIterator
	{
		const_iterator iterator_;
		const_iterator end_;

	public:
		LegacyIterator(const UnorderedSet *set)
			: iterator_(set->begin()), end_(set->end())
		{
		}

		BOOL
		Advance()
		{
			if (iterator_ == end_)
			{
				return false;
			}
			++iterator_;
			return true;
		}

		const Key &
		Get() const noexcept
		{
			return *std::prev(iterator_);
		}
	};
};
}  // namespace gpos

#endif	//GPOS_UnorderedSet_H
