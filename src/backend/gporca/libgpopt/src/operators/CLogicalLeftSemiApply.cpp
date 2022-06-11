//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalLeftSemiApply.cpp
//
//	@doc:
//		Implementation of left semi-apply operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalLeftSemiApply.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiApply::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalLeftSemiApply::DeriveMaxCard(CMemoryPool *,	 // mp
									 CExpressionHandle &exprhdl) const
{
	return CLogical::Maxcard(exprhdl, 2 /*ulScalarIndex*/,
							 exprhdl.DeriveMaxCard(0));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiApply::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalLeftSemiApply::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);

	(void) xform_set->ExchangeSet(CXform::ExfLeftSemiApply2LeftSemiJoin);
	(void) xform_set->ExchangeSet(
		CXform::ExfLeftSemiApplyWithExternalCorrs2InnerJoin);
	(void) xform_set->ExchangeSet(
		CXform::ExfLeftSemiApply2LeftSemiJoinNoCorrelations);

	return xform_set;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiApply::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalLeftSemiApply::DeriveOutputColumns(CMemoryPool *,  // mp
										   CExpressionHandle &exprhdl)
{
	GPOS_ASSERT(3 == exprhdl.Arity());

	return PcrsDeriveOutputPassThru(exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiApply::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalLeftSemiApply::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrInner =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrInner, colref_mapping, must_exist);

	return GPOS_NEW(mp)
		CLogicalLeftSemiApply(mp, std::move(pdrgpcrInner), m_eopidOriginSubq);
}

// EOF
