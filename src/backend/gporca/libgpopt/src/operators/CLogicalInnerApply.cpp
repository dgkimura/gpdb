//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalInnerApply.cpp
//
//	@doc:
//		Implementation of inner apply operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalInnerApply.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerApply::CLogicalInnerApply
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalInnerApply::CLogicalInnerApply(CMemoryPool *mp) : CLogicalApply(mp)
{
	GPOS_ASSERT(nullptr != mp);

	m_fPattern = true;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerApply::CLogicalInnerApply
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalInnerApply::CLogicalInnerApply(CMemoryPool *mp,
									   gpos::owner<CColRefArray *> pdrgpcrInner,
									   EOperatorId eopidOriginSubq)
	: CLogicalApply(mp, std::move(pdrgpcrInner), eopidOriginSubq)
{
	GPOS_ASSERT(0 < pdrgpcrInner->Size());
}



//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerApply::~CLogicalInnerApply
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalInnerApply::~CLogicalInnerApply() = default;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerApply::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalInnerApply::DeriveMaxCard(CMemoryPool *,  // mp
								  CExpressionHandle &exprhdl) const
{
	return CLogical::Maxcard(exprhdl, 2 /*ulScalarIndex*/, MaxcardDef(exprhdl));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerApply::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalInnerApply::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);

	(void) xform_set->ExchangeSet(CXform::ExfInnerApply2InnerJoin);
	(void) xform_set->ExchangeSet(
		CXform::ExfInnerApply2InnerJoinNoCorrelations);
	(void) xform_set->ExchangeSet(CXform::ExfInnerApplyWithOuterKey2InnerJoin);

	return xform_set;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalInnerApply::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalInnerApply::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrInner =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrInner, colref_mapping, must_exist);

	return GPOS_NEW(mp)
		CLogicalInnerApply(mp, std::move(pdrgpcrInner), m_eopidOriginSubq);
}

// EOF
