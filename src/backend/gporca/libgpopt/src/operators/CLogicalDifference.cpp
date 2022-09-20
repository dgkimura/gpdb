//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDifference.cpp
//
//	@doc:
//		Implementation of Difference operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDifference.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::CLogicalDifference
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDifference::CLogicalDifference(CMemoryPool *mp) : CLogicalSetOp(mp)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::CLogicalDifference
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDifference::CLogicalDifference(
	CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput)
	: CLogicalSetOp(mp, std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrInput))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::~CLogicalDifference
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDifference::~CLogicalDifference() = default;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalDifference::DeriveMaxCard(CMemoryPool *,  // mp
								  CExpressionHandle &exprhdl) const
{
	// contradictions produce no rows
	if (exprhdl.DerivePropertyConstraint()->FContradiction())
	{
		return CMaxCard(0 /*ull*/);
	}

	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalDifference::PopCopyWithRemappedColumns(CMemoryPool *mp,
											   UlongToColRefMap *colref_mapping,
											   BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrOutput = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrOutput.get(), colref_mapping, must_exist);
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrInput.get(), colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalDifference(mp, std::move(pdrgpcrOutput),
										   std::move(pdrgpdrgpcrInput));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalDifference::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfDifference2LeftAntiSemiJoin);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalDifference::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
								 IStatisticsArray *	 // not used
) const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	// difference is transformed into an aggregate over a LASJ,
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

	IStatistics *outer_stats = exprhdl.Pstats(0);
	IStatistics *inner_side_stats = exprhdl.Pstats(1);

	// construct the scalar condition for the LASJ
	gpos::Ref<CExpression> pexprScCond =
		CUtils::PexprConjINDFCond(mp, m_pdrgpdrgpcrInput.get());

	// compute the statistics for LASJ
	CColRefSet *outer_refs = exprhdl.DeriveOuterReferences();
	gpos::Ref<CStatsPredJoinArray> join_preds_stats =
		CStatsPredUtils::ExtractJoinStatsFromExpr(
			mp, exprhdl, pexprScCond.get(), output_colrefsets.get(), outer_refs,
			true  // is an LASJ
		);
	gpos::Ref<IStatistics> LASJ_stats = outer_stats->CalcLASJoinStats(
		mp, inner_side_stats, join_preds_stats.get(),
		true /* DoIgnoreLASJHistComputation */
	);

	// clean up
	;
	;

	// computed columns
	gpos::Ref<ULongPtrArray> pdrgpulComputedCols =
		GPOS_NEW(mp) ULongPtrArray(mp);
	gpos::Ref<IStatistics> stats = CLogicalGbAgg::PstatsDerive(
		mp, LASJ_stats.get(),
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
