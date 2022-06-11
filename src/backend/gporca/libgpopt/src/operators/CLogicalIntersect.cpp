//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalIntersect.cpp
//
//	@doc:
//		Implementation of Intersect operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalIntersect.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalIntersectAll.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersect::CLogicalIntersect
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalIntersect::CLogicalIntersect(CMemoryPool *mp) : CLogicalSetOp(mp)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersect::CLogicalIntersect
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalIntersect::CLogicalIntersect(CMemoryPool *mp,
									 gpos::Ref<CColRefArray> pdrgpcrOutput,
									 gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput)
	: CLogicalSetOp(mp, std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrInput))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersect::~CLogicalIntersect
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalIntersect::~CLogicalIntersect() = default;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersect::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalIntersect::DeriveMaxCard(CMemoryPool *,	 // mp
								 CExpressionHandle &exprhdl) const
{
	// contradictions produce no rows
	if (exprhdl.DerivePropertyConstraint()->FContradiction())
	{
		return CMaxCard(0 /*ull*/);
	}

	CMaxCard maxcardL = exprhdl.DeriveMaxCard(0);
	CMaxCard maxcardR = exprhdl.DeriveMaxCard(1);

	if (maxcardL <= maxcardR)
	{
		return maxcardL;
	}

	return maxcardR;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersect::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalIntersect::PopCopyWithRemappedColumns(CMemoryPool *mp,
											  UlongToColRefMap *colref_mapping,
											  BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrOutput = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrOutput.get(), colref_mapping, must_exist);
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrInput.get(), colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalIntersect(mp, std::move(pdrgpcrOutput),
										  std::move(pdrgpdrgpcrInput));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersect::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalIntersect::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfIntersect2Join);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersect::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalIntersect::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
								IStatisticsArray *	// not used
) const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	// intersect is transformed into a group by over an intersect all
	// we follow the same route to compute statistics

	gpos::Ref<CColRefSetArray> output_colrefsets =
		GPOS_NEW(mp) CColRefSetArray(mp);
	const ULONG size = m_pdrgpdrgpcrInput->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CColRefSet> pcrs =
			GPOS_NEW(mp) CColRefSet(mp, (*m_pdrgpdrgpcrInput)[ul].get());
		output_colrefsets->Append(pcrs);
	}

	gpos::Ref<IStatistics> pstatsIntersectAll =
		CLogicalIntersectAll::PstatsDerive(
			mp, exprhdl, m_pdrgpdrgpcrInput.get(), output_colrefsets.get());

	// computed columns
	gpos::Ref<ULongPtrArray> pdrgpulComputedCols =
		GPOS_NEW(mp) ULongPtrArray(mp);

	gpos::Ref<IStatistics> stats = CLogicalGbAgg::PstatsDerive(
		mp, pstatsIntersectAll.get(),
		(*m_pdrgpdrgpcrInput)[0]
			.get(),	 // we group by the columns of the first child
		pdrgpulComputedCols.get(),	// no computed columns for set ops
		nullptr						// no keys, use all grouping cols
	);
	// clean up
	;
	;
	;

	return stats;
}

// EOF
