//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalLeftSemiCorrelatedApply.cpp
//
//	@doc:
//		Implementation of left semi correlated apply for EXISTS subqueries
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalLeftSemiCorrelatedApply.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApply::CLogicalLeftSemiCorrelatedApply
//
//	@doc:
//		Ctor - for patterns
//
//---------------------------------------------------------------------------
CLogicalLeftSemiCorrelatedApply::CLogicalLeftSemiCorrelatedApply(
	CMemoryPool *mp)
	: CLogicalLeftSemiApply(mp)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApply::CLogicalLeftSemiCorrelatedApply
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalLeftSemiCorrelatedApply::CLogicalLeftSemiCorrelatedApply(
	CMemoryPool *mp, gpos::owner<CColRefArray *> pdrgpcrInner,
	EOperatorId eopidOriginSubq)
	: CLogicalLeftSemiApply(mp, std::move(pdrgpcrInner), eopidOriginSubq)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApply::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalLeftSemiCorrelatedApply::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementLeftSemiCorrelatedApply);

	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApply::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalLeftSemiCorrelatedApply::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrInner =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrInner, colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalLeftSemiCorrelatedApply(
		mp, std::move(pdrgpcrInner), m_eopidOriginSubq);
}


// EOF
