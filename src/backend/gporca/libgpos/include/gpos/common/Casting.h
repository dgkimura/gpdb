//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_Casting_H
#define GPOS_Casting_H

#include <utility>	// for exchange

#include "gpos/assert.h"
#include "gpos/common/Ref.h"
#include "gpos/error/CException.h"

namespace gpos
{
namespace internal
{
// base template: check at runtime
template <class To, class From, class Enabler = void>
struct isa_impl
{
	static inline bool
	isa(const From *f)
	{
		return dynamic_cast<const To *>(f) != nullptr;
	}
};

// upcast: we can determine this at compile-time
template <class To, class From>
struct isa_impl<
	To, From,
	std::enable_if_t<std::is_base_of<To, std::remove_cv_t<From>>::value>>
{
	static inline bool
	isa(From *)
	{
		return true;
	}
};
}  // namespace internal

template <class T, class F>
inline bool
isa(F *f)
{
	return internal::isa_impl<T, F>::isa(f);
}

template <class T, class F>
inline bool
isa(const Ref<F> &f)
{
	return isa<T>(f.get());
}

// helper template to "graft" the const qualifier
template <class To, class From>
struct cast_return;

template <class To, class From>
using cast_return_t = typename cast_return<To, From>::type;

template <class To, class From>
struct cast_return<To, From *>
{
	using type = To *;
};

template <class To, class From>
struct cast_return<To, const From *>
{
	using type = const To *;
};

template <class T, class F>
inline auto
cast(F *f)
{
	GPOS_ASSERT(isa<T>(f) && "incompatible argument to cast<T>()");
	return static_cast<cast_return_t<T, F *>>(f);
}

template <class T, class F>
inline Ref<T>
cast(Ref<F> &&f)
{
	GPOS_ASSERT(isa<T>(f) && "incompatible argument to cast<T>()");
	Ref<T> ret;
	ret.p_ = cast<T>(std::exchange(f.p_, nullptr));
	return ret;
}

template <class T, class F>
inline Ref<T>
cast(const Ref<F> &f)
{
	GPOS_ASSERT(isa<T>(f) && "incompatible argument to cast<T>()");
	return {cast<T>(f.get())};
}

// FIXME: caller should pass in non-null input
template <class T, class F>
inline auto
dyn_cast(F *f)
{
	return dynamic_cast<cast_return_t<T, F *>>(f);
}

template <class T, class F>
inline Ref<T>
dyn_cast(const Ref<F> &f)
{
	return {dyn_cast<T>(f.get())};
}

template <class T, class F>
inline Ref<T>
dyn_cast(Ref<F> &&f)
{
	if (!isa<T>(f))
	{
		return nullptr;
	}

	return cast<T>(std::move(f));
}

}  // namespace gpos
#endif	// GPOS_Casting_H
