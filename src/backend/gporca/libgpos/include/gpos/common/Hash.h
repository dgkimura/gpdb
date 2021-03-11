//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_Hash_H
#define GPOS_Hash_H

#include "gpos/types.h"	 // for ULONG

namespace gpos
{
template <class K, ULONG (*HashFn)(const K *)>
struct PtrHash
{
	ULONG
	operator()(const K *k) const noexcept
	{
		return HashFn(k);
	}
};

template <class K, BOOL (*EqFn)(const K *, const K *)>
struct PtrEqual
{
	BOOL
	operator()(const K *lhs, const K *rhs) const noexcept
	{
		return EqFn(lhs, rhs);
	}
};

}  // namespace gpos

#endif	//GPOS_Hash_H
