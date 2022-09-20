//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalLeftSemiCorrelatedApplyIn.cpp
//
//	@doc:
//		Implementation of left semi correlated apply with IN semantics
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApplyIn::CLogicalLeftSemiCorrelatedApplyIn
//
//	@doc:
//		Ctor - for patterns
//
//---------------------------------------------------------------------------
CLogicalLeftSemiCorrelatedApplyIn::CLogicalLeftSemiCorrelatedApplyIn(
	CMemoryPool *mp)
	: CLogicalLeftSemiApplyIn(mp)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApplyIn::CLogicalLeftSemiCorrelatedApplyIn
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalLeftSemiCorrelatedApplyIn::CLogicalLeftSemiCorrelatedApplyIn(
	CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrInner,
	EOperatorId eopidOriginSubq)
	: CLogicalLeftSemiApplyIn(mp, std::move(pdrgpcrInner), eopidOriginSubq)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApplyIn::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalLeftSemiCorrelatedApplyIn::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(
		CXform::ExfImplementLeftSemiCorrelatedApplyIn);

	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiCorrelatedApplyIn::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalLeftSemiCorrelatedApplyIn::PopCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrInner = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrInner.get(), colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalLeftSemiCorrelatedApplyIn(
		mp, std::move(pdrgpcrInner), m_eopidOriginSubq);
}


// EOF
