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
	CMemoryPool *mp, gpos::owner<CColRefArray *> pdrgpcrOutput,
	gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput)
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
gpos::owner<COperator *>
CLogicalIntersectAll::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrOutput =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrOutput, colref_mapping, must_exist);
	gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrInput, colref_mapping, must_exist);

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
gpos::owner<CKeyCollection *>
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
gpos::owner<CXformSet *>
CLogicalIntersectAll::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
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
gpos::owner<IStatistics *>
CLogicalIntersectAll::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput,
	gpos::pointer<CColRefSetArray *>
		output_colrefsets  // output of relational children
)
{
	GPOS_ASSERT(2 == exprhdl.Arity());

	gpos::pointer<IStatistics *> outer_stats = exprhdl.Pstats(0);
	gpos::pointer<IStatistics *> inner_side_stats = exprhdl.Pstats(1);

	// construct the scalar condition similar to transform that turns an "intersect all" into a "left semi join"
	// over a window operation on the individual input (for row_number)

	// TODO:  Jan 8th 2012, add the stats for window operation
	gpos::owner<CExpression *> pexprScCond =
		CUtils::PexprConjINDFCond(mp, pdrgpdrgpcrInput);
	CColRefSet *outer_refs = exprhdl.DeriveOuterReferences();
	gpos::owner<CStatsPredJoinArray *> join_preds_stats =
		CStatsPredUtils::ExtractJoinStatsFromExpr(mp, exprhdl, pexprScCond,
												  output_colrefsets, outer_refs,
												  true	// is a semi-join
		);
	gpos::owner<IStatistics *> pstatsSemiJoin =
		CLogicalLeftSemiJoin::PstatsDerive(mp, join_preds_stats, outer_stats,
										   inner_side_stats);

	// clean up
	pexprScCond->Release();
	join_preds_stats->Release();

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
gpos::owner<IStatistics *>
CLogicalIntersectAll::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<IStatisticsArray *>  // not used
) const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	gpos::owner<CColRefSetArray *> output_colrefsets =
		GPOS_NEW(mp) CColRefSetArray(mp);
	const ULONG size = m_pdrgpdrgpcrInput->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::owner<CColRefSet *> pcrs =
			GPOS_NEW(mp) CColRefSet(mp, (*m_pdrgpdrgpcrInput)[ul]);
		output_colrefsets->Append(pcrs);
	}
	gpos::owner<IStatistics *> stats =
		PstatsDerive(mp, exprhdl, m_pdrgpdrgpcrInput, output_colrefsets);

	// clean up
	output_colrefsets->Release();

	return stats;
}

// EOF
