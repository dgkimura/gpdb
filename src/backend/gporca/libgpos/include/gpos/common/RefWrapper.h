
//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2023 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_RefWrapper_H
#define GPOS_RefWrapper_H

#include "gpos/common/CDouble.h"
#include "gpos/common/CRefCount.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/types.h"	 // for ULONG

namespace gpos
{
class UlongRef : public CRefCount<UlongRef>
{
	ULONG m_ul;

public:
	UlongRef(ULONG ul) noexcept : m_ul(ul)
	{
	}
	~UlongRef() = default;

	ULONG
	value()
	{
		return m_ul;
	}
};

template <class T>
class WrapperRef : public CRefCount<WrapperRef<T>>
{
	T m_obj;

public:
	WrapperRef(T obj) noexcept : m_obj(obj)
	{
	}
	~WrapperRef() = default;

	T
	value()
	{
		return m_obj;
	}
};

using WrapperCDouble = gpos::WrapperRef<CDouble>;

template <class T>
class WrapperRaw : public CRefCount<WrapperRaw<T>>
{
	T *m_obj;

public:
	WrapperRaw(T *obj) noexcept : m_obj(obj)
	{
	}
	~WrapperRaw()
	{
		GPOS_DELETE(m_obj);
	}

	T *
	value()
	{
		return m_obj;
	}
};

using WrapperCWStringDynamic = gpos::WrapperRaw<CWStringDynamic>;

}  // namespace gpos

#endif	// !GPOS_RefWrapper_H
