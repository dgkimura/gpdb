//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_Ref_H
#define GPOS_Ref_H
#include <type_traits>
#include <typeindex>  // for hash
#include <utility>	  // for exchange

#include "gpos/common/Hash.h"  // FIXME: sneaky
#include "gpos/types.h"		   // for ULONG

namespace gpos
{
// Specialize this in case of a forward-declared T
template <class T>
struct RefPtrInfo
{
	static void
	AddRef(T *t) noexcept
	{
		t->AddRef();
	}

	static void
	Release(T *t) noexcept(std::is_nothrow_destructible<T>::value)
	{
		t->Release();
	}
};

template <class T>
class Ref
{
	T *p_;

	void
	AddRef() noexcept
	{
		if (p_)
			RefPtrInfo<T>::AddRef(p_);
	}

	void
	Release() noexcept(noexcept(RefPtrInfo<T>::AddRef(p_)))
	{
		if (p_)
			RefPtrInfo<T>::Release(p_);
	}

	template <class U>
	friend class Ref;  // for swap and move

	template <class To, class From>
	friend Ref<To> cast(Ref<From> &&);

	template <class U>
	friend Ref RefFromNew(U *);
	// attach to a pointer that already has a reference. transitional helper
	// before we change CRefCount to initialize at 0.
	// FIXME: fix CRefCount
	void
	Attach(T *p) noexcept
	{
		p_ = p;
	}

public:
	Ref(T *p = nullptr) noexcept : p_(p)
	{
		AddRef();
	}
	~Ref()
	{
		Release();
	}

	Ref(const Ref &other) noexcept : p_(other.get())
	{
		AddRef();
	}

	template <class U,
			  class = std::enable_if_t<std::is_convertible<U *, T *>::value>>
	Ref(const Ref<U> &other) noexcept : p_(other.get())
	{
		AddRef();
	}

	template <class U,
			  class = std::enable_if_t<std::is_convertible<U *, T *>::value>>
	Ref(Ref<U> &&other) noexcept : p_(std::exchange(other.p_, nullptr))
	{
	}

	friend void
	swap(Ref &lhs, Ref &rhs) noexcept
	{
		std::swap(lhs.p_, rhs.p_);
	}

	Ref &
	operator=(Ref other) noexcept
	{
		swap(*this, other);
		return *this;
	}

	explicit operator bool() const noexcept
	{
		return p_;
	}

	T *
	operator->() const noexcept
	{
		return p_;
	}

	T &
	operator*() const noexcept
	{
		return *p_;
	}

	bool
	operator!=(const Ref &other) const noexcept
	{
		return p_ != other.p_;
	}

	bool
	operator==(const Ref &other) const noexcept
	{
		return p_ == other.p_;
	}

	friend bool
	operator!=(const Ref &ref, const T *other) noexcept
	{
		return ref.p_ != other;
	}

	friend bool
	operator==(const Ref &ref, const T *other) noexcept
	{
		return ref.p_ == other;
	}

	friend bool
	operator!=(const T *other, const Ref &ref) noexcept
	{
		return ref != other;
	}

	friend bool
	operator==(const T *other, const Ref &ref) noexcept
	{
		return ref == other;
	}

	T *
	get() const noexcept
	{
		return p_;
	}
};

template <class T>
Ref<T>
RefFromNew(T *p) noexcept
{
	Ref<T> ref;
	ref.Attach(p);
	return ref;
}

// Adapter to turn a legacy hash function for CHashMap into a functor for
// std::unordered_map
template <class K, ULONG (*HashFn)(const K *)>
struct RefHash
{
	ULONG
	operator()(const Ref<K> &k) const noexcept
	{
		return HashFn(k.get());
	}
};

// Adapter to turn a legacy equal function for CHashMap into a functor for the
// KeyEqual parameter for std::unordered_map
template <class K, BOOL (*EqFn)(const K *, const K *)>
struct RefEq
{
	BOOL
	operator()(const Ref<K> &lhs, const Ref<K> &rhs) const noexcept
	{
		return EqFn(lhs.get(), rhs.get());
	}
};

}  // namespace gpos

template <class T>
struct std::hash<gpos::Ref<T>>
{
	std::size_t
	operator()(const gpos::Ref<T> &ref) const noexcept
	{
		return std::hash<T *>{}(ref.get());
	}
};

#define REF_PTR_TYPE_ADDREF_RELEASE_FWD_DECL(NS, TYPE, ADD_REF, RELEASE) \
	namespace NS                                                         \
	{                                                                    \
	class TYPE;                                                          \
	void ADD_REF(TYPE *x) noexcept;                                      \
	void RELEASE(TYPE *x) noexcept;                                      \
	}                                                                    \
                                                                         \
	template <>                                                          \
	struct gpos::RefPtrInfo<NS::TYPE>                                    \
	{                                                                    \
		static void                                                      \
		AddRef(NS::TYPE *x) noexcept                                     \
		{                                                                \
			NS::ADD_REF(x);                                              \
		}                                                                \
		static void                                                      \
		Release(NS::TYPE *x) noexcept                                    \
		{                                                                \
			NS::RELEASE(x);                                              \
		}                                                                \
	}

// Common case, we name the methods after a convention
#define REF_PTR_FWD_DECL(NS, TYPE) \
	REF_PTR_TYPE_ADDREF_RELEASE_FWD_DECL(NS, TYPE, AddRef##TYPE, Release##TYPE)

// Drop this in namespace scope in the header file that declares TYPE
#define REF_PTR_ADDREF_RELEASE_INLINE_DEF(TYPE) \
	inline void AddRef##TYPE(TYPE *x) noexcept  \
	{                                           \
		x->AddRef();                            \
	}                                           \
	inline void Release##TYPE(TYPE *x) noexcept \
	{                                           \
		x->Release();                           \
	}                                           \
	static_assert(true, "")

#endif
