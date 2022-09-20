//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalIntersectAll.cpp
//
//	@doc:
//		Implementation of Intersect all operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalIntersectAll.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::CLogicalIntersectAll
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalIntersectAll::CLogicalIntersectAll(CMemoryPool *mp) : CLogicalSetOp(mp)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::CLogicalIntersectAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalIntersectAll::CLogicalIntersectAll(
	CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput)
	: CLogicalSetOp(mp, std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrInput))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::~CLogicalIntersectAll
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalIntersectAll::~CLogicalIntersectAll() = default;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalIntersectAll::DeriveMaxCard(CMemoryPool *,	// mp
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
//		CLogicalIntersectAll::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalIntersectAll::PopCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrOutput = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrOutput.get(), colref_mapping, must_exist);
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrInput.get(), colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalIntersectAll(mp, std::move(pdrgpcrOutput),
											 std::move(pdrgpdrgpcrInput));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::Ref<CKeyCollection>
CLogicalIntersectAll::DeriveKeyCollection(CMemoryPool *,	   //mp,
										  CExpressionHandle &  //exprhdl
) const
{
	// TODO: Add the keys from outer and inner child
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalIntersectAll::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfIntersectAll2LeftSemiJoin);

	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalIntersectAll::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	CColRef2dArray *pdrgpdrgpcrInput,
	CColRefSetArray *output_colrefsets	// output of relational children
)
{
	GPOS_ASSERT(2 == exprhdl.Arity());

	IStatistics *outer_stats = exprhdl.Pstats(0);
	IStatistics *inner_side_stats = exprhdl.Pstats(1);

	// construct the scalar condition similar to transform that turns an "intersect all" into a "left semi join"
	// over a window operation on the individual input (for row_number)

	// TODO:  Jan 8th 2012, add the stats for window operation
	gpos::Ref<CExpression> pexprScCond =
		CUtils::PexprConjINDFCond(mp, pdrgpdrgpcrInput);
	CColRefSet *outer_refs = exprhdl.DeriveOuterReferences();
	gpos::Ref<CStatsPredJoinArray> join_preds_stats =
		CStatsPredUtils::ExtractJoinStatsFromExpr(
			mp, exprhdl, pexprScCond.get(), output_colrefsets, outer_refs,
			true  // is a semi-join
		);
	gpos::Ref<IStatistics> pstatsSemiJoin = CLogicalLeftSemiJoin::PstatsDerive(
		mp, join_preds_stats.get(), outer_stats, inner_side_stats);

	// clean up
	;
	;

	return pstatsSemiJoin;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalIntersectAll::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
								   IStatisticsArray *  // not used
) const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	gpos::Ref<CColRefSetArray> output_colrefsets =
		GPOS_NEW(mp) CColRefSetArray(mp);
	const ULONG size = m_pdrgpdrgpcrInput->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CColRefSet> pcrs =
			GPOS_NEW(mp) CColRefSet(mp, (*m_pdrgpdrgpcrInput)[ul].get());
		output_colrefsets->Append(pcrs);
	}
	gpos::Ref<IStatistics> stats = PstatsDerive(
		mp, exprhdl, m_pdrgpdrgpcrInput.get(), output_colrefsets.get());

	// clean up
	;

	return stats;
}

// EOF
