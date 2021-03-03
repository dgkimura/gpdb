//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2020 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_Vector_H
#define GPOS_Vector_H

#include <algorithm>
#include <type_traits>

#include "gpos/common/CRefCount.h"
#include "gpos/memory/vector.h"

namespace gpos
{
template <class T>
class Vector : public gpos::CRefCount
{
	gpos::vector<T> v_;

public:
	// ctor
	explicit Vector(CMemoryPool *mp, ULONG min_size GPOS_UNUSED = 4,
					ULONG expansion_factor GPOS_UNUSED = 10)
		: v_(mp)
	{
	}

	// clear elements
	void
	Clear() noexcept
	{
		v_.clear();
	}

	// append element to end of array
	void
	Append(T elem)
	{
		v_.push_back(std::move(elem));
	}

	// number of elements currently held
	ULONG
	Size() const noexcept
	{
		return v_.size();
	}

	// sort array
	void
	Sort() noexcept
	{
		std::sort(v_.begin(), v_.end());
	}

	template <class Compare>
	void
	Sort(Compare comp)
	{
		std::sort(v_.begin(), v_.end(), comp);
	}

	// This is retained for backward compatibility, and only generated when T is
	// a pointer type
	// FIXME: this API is not type safe, migrate all callers to std::sort
	template <class Compare>
	std::enable_if_t<std::is_same<clib::Comparator, Compare>::value &&
					 std::is_pointer<T>::value>
	Sort(Compare compare_func) noexcept(noexcept(compare_func))
	{
		Sort([compare_func](T a, T b) { return compare_func(&a, &b) < 0; });
	}

	// equality check
	BOOL
	Equals(const Vector *arr) const noexcept
	{
		return std::equal(v_.begin(), v_.end(), arr->v_.begin(), arr->v_.end());
	}

#ifdef GPOS_DEBUG
	// check if array is sorted
	BOOL
	IsSorted() const noexcept
	{
		return std::is_sorted(v_.begin(), v_.end());
	}
#endif	// GPOS_DEBUG

	// accessor for n-th element
	const T &
	operator[](ULONG pos) const noexcept
	{
		return v_[pos];
	}

	T &
	operator[](ULONG pos) noexcept
	{
		return v_[pos];
	}

	// swap two array entries
	void
	Swap(ULONG pos1, ULONG pos2) noexcept
	{
		GPOS_ASSERT(pos1 < v_.size() && pos2 < v_.size() &&
					"Swap positions out of bounds");
		using std::iter_swap;
		iter_swap(v_.begin() + pos1, v_.begin() + pos2);
	}

	// We have to erase this method when the element type is not a pointer-like,
	// because "return nullptr" would otherwise be meaningless.
	// FIXME: this is only used by CKHeap, gut this method and replace CKHeap
	// with std lib return the last element of the array and at the same time
	// remove it from the array
	T
	RemoveLast()
	{
		static_assert(std::is_convertible<std::nullptr_t, T>::value,
					  "T has to be implicitly convertible from nullptr");

		if (v_.empty())
		{
			return nullptr;
		}

		T t = std::move(v_.back());
		v_.pop_back();

		return t;
	}

	void
	Replace(ULONG pos, T new_elem) noexcept
	{
		v_[pos] = std::move(new_elem);
	}

	auto
	begin() noexcept
	{
		return v_.begin();
	}

	auto
	begin() const noexcept
	{
		return v_.begin();
	}

	auto
	end() noexcept
	{
		return v_.end();
	}

	auto
	end() const noexcept
	{
		return v_.end();
	}
};
}  // namespace gpos
#endif	// GPOS_Vector_H
