//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_ranges_H
#define GPOS_ranges_H

#include <iterator>

namespace gpos
{
template <class It>
using iter_difference_t = typename std::iterator_traits<
	std::remove_cv_t<std::remove_reference_t<It>>>::difference_type;

namespace internal
{
// SubRange
template <class It>
class subrange_t
{
	It begin_, end_;

public:
	constexpr bool
	empty() const
	{
		return begin_ == end_;
	}

	constexpr It
	begin() const
	{
		return begin_;
	}

	constexpr It
	end() const
	{
		return end_;
	}

	constexpr iter_difference_t<It>
	size() const
	{
		return std::distance(begin_, end_);
	}

	constexpr subrange_t &
	advance(iter_difference_t<It> n)
	{
		std::advance(begin_, n);
		return *this;
	}

	constexpr subrange_t
	next(iter_difference_t<It> n) const &
	{
		auto ret = *this;
		ret.advance(n);
		return ret;
	}

	constexpr subrange_t
	next(iter_difference_t<It> n) &&
	{
		advance(n);
		return std::move(*this);
	}

	constexpr subrange_t(It begin, It end)
		: begin_(std::move(begin)), end_(std::move(end))
	{
	}
};
}  // namespace internal

/// Poor man's std::ranges::subrange in pre C++20:
///
/// N.B. helper functions like these (and std::make_pair) are only necessary in
/// C++ 14 and before, because constructor calls will have proper template
/// argument deduction once we bump to C++ 17.
/// TODO: rename the class to \c subrange and remove the helper function once we
/// bump to C++ 17
template <class It>
auto
subrange(It begin, It end)
{
	return internal::subrange_t<It>(begin, end);
}

template <class Container>
auto
subrange(Container &&c)
{
	using std::begin;
	using std::end;
	return internal::subrange_t<decltype(begin(c))>(begin(c), end(c));
}
}  // namespace gpos

#endif	// GPOS_ranges_H
