//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalLeftSemiNLJoin.cpp
//
//	@doc:
//		Implementation of left semi nested-loops join operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalLeftSemiNLJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLeftSemiNLJoin::CPhysicalLeftSemiNLJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalLeftSemiNLJoin::CPhysicalLeftSemiNLJoin(CMemoryPool *mp)
	: CPhysicalNLJoin(mp)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLeftSemiNLJoin::~CPhysicalLeftSemiNLJoin
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalLeftSemiNLJoin::~CPhysicalLeftSemiNLJoin() = default;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLeftSemiNLJoin::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalLeftSemiNLJoin::FProvidesReqdCols(
	CExpressionHandle &exprhdl, gpos::pointer<CColRefSet *> pcrsRequired,
	ULONG  // ulOptReq
) const
{
	// left semi join only propagates columns from left child
	return FOuterProvidesReqdCols(exprhdl, pcrsRequired);
}


// EOF
