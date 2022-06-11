//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarArrayRefIndexList.cpp
//
//	@doc:
//		Implementation of scalar arrayref index list
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarArrayRefIndexList.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRefIndexList::CScalarArrayRefIndexList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarArrayRefIndexList::CScalarArrayRefIndexList(CMemoryPool *mp,
												   EIndexListType eilt)
	: CScalar(mp), m_eilt(eilt)
{
	GPOS_ASSERT(EiltSentinel > eilt);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRefIndexList::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarArrayRefIndexList::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	gpos::pointer<CScalarArrayRefIndexList *> popIndexList =
		gpos::dyn_cast<CScalarArrayRefIndexList>(pop);

	return m_eilt == popIndexList->Eilt();
}

// EOF
