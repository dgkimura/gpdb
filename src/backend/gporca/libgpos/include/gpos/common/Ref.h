//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_Ref_H
#define GPOS_Ref_H
#include <type_traits>
#include <utility>	// for exchange

namespace gpos
{
template <class T>
class Ref
{
	T *p_;

	void
	Release() noexcept(std::is_nothrow_destructible<T>::value)
	{
		if (p_)
			p_->Release();
	}

	void
	AddRef() noexcept
	{
		if (p_)
			p_->AddRef();
	}

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

	Ref(const Ref &other) noexcept : p_(other.p_)
	{
		AddRef();
	}
	Ref(Ref &&other) noexcept : p_(std::exchange(other.p_, nullptr))
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
}  // namespace gpos
#endif
