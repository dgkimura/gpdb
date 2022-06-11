//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalInnerCorrelatedApply.cpp
//
//	@doc:
//		Implementation of inner correlated apply operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalInnerCorrelatedApply.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerCorrelatedApply::CLogicalInnerCorrelatedApply
//
//	@doc:
//		Ctor - for patterns
//
//---------------------------------------------------------------------------
CLogicalInnerCorrelatedApply::CLogicalInnerCorrelatedApply(CMemoryPool *mp)
	: CLogicalInnerApply(mp)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerCorrelatedApply::CLogicalInnerCorrelatedApply
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalInnerCorrelatedApply::CLogicalInnerCorrelatedApply(
	CMemoryPool *mp, gpos::owner<CColRefArray *> pdrgpcrInner,
	EOperatorId eopidOriginSubq)
	: CLogicalInnerApply(mp, std::move(pdrgpcrInner), eopidOriginSubq)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerCorrelatedApply::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalInnerCorrelatedApply::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementInnerCorrelatedApply);

	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerCorrelatedApply::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalInnerCorrelatedApply::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() == Eopid())
	{
		return m_pdrgpcrInner->Equals(
			gpos::dyn_cast<CLogicalInnerCorrelatedApply>(pop)->PdrgPcrInner());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerCorrelatedApply::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalInnerCorrelatedApply::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrInner =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrInner, colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalInnerCorrelatedApply(
		mp, std::move(pdrgpcrInner), m_eopidOriginSubq);
}

// EOF
