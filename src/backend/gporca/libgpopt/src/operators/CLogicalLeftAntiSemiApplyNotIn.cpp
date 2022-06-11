//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CLogicalLeftAntiSemiApplyNotIn.cpp
//
//	@doc:
//		Implementation of left anti-semi-apply operator with NotIn semantics
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiApplyNotIn::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalLeftAntiSemiApplyNotIn::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(
		CXform::ExfLeftAntiSemiApplyNotIn2LeftAntiSemiJoinNotIn);
	(void) xform_set->ExchangeSet(
		CXform::ExfLeftAntiSemiApplyNotIn2LeftAntiSemiJoinNotInNoCorrelations);

	return xform_set;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiApplyNotIn::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalLeftAntiSemiApplyNotIn::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrInner =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrInner, colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalLeftAntiSemiApplyNotIn(
		mp, std::move(pdrgpcrInner), m_eopidOriginSubq);
}

// EOF
