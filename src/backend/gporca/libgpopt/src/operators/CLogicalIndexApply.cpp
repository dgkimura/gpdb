//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	Implementation of inner / left outer index apply operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalIndexApply.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/statistics/CJoinStatsProcessor.h"

using namespace gpopt;

CLogicalIndexApply::CLogicalIndexApply(CMemoryPool *mp)
	: CLogicalApply(mp),
	  m_pdrgpcrOuterRefs(nullptr),
	  m_fOuterJoin(false),
	  m_origJoinPred(nullptr)
{
	m_fPattern = true;
}

CLogicalIndexApply::CLogicalIndexApply(CMemoryPool *mp,
									   gpos::Ref<CColRefArray> pdrgpcrOuterRefs,
									   BOOL fOuterJoin,
									   gpos::Ref<CExpression> origJoinPred)
	: CLogicalApply(mp),
	  m_pdrgpcrOuterRefs(pdrgpcrOuterRefs),
	  m_fOuterJoin(fOuterJoin),
	  m_origJoinPred(origJoinPred)
{
	GPOS_ASSERT(nullptr != m_pdrgpcrOuterRefs);
	if (nullptr != m_origJoinPred)
	{
		// We don't allow subqueries in the expression that we
		// store in the logical operator, since such expressions
		// would be unsuitable for generating a plan.
		GPOS_RTL_ASSERT(!m_origJoinPred->DeriveHasSubquery());
		;
	}
}


CLogicalIndexApply::~CLogicalIndexApply()
{
	;
	;
}


CMaxCard
CLogicalIndexApply::DeriveMaxCard(CMemoryPool *,  // mp
								  CExpressionHandle &exprhdl) const
{
	return CLogical::Maxcard(exprhdl, 2 /*ulScalarIndex*/, MaxcardDef(exprhdl));
}


gpos::Ref<CXformSet>
CLogicalIndexApply::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementIndexApply);
	return xform_set;
}

BOOL
CLogicalIndexApply::Matches(COperator *pop) const
{
	GPOS_ASSERT(nullptr != pop);

	if (pop->Eopid() == Eopid())
	{
		return m_pdrgpcrOuterRefs->Equals(
			gpos::dyn_cast<CLogicalIndexApply>(pop)->PdrgPcrOuterRefs());
	}

	return false;
}


gpos::Ref<IStatistics>
CLogicalIndexApply::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
								 IStatisticsArray *	 // stats_ctxt
) const
{
	GPOS_ASSERT(EspNone < Esp(exprhdl));

	IStatistics *outer_stats = exprhdl.Pstats(0);
	IStatistics *inner_side_stats = exprhdl.Pstats(1);
	CExpression *pexprScalar = exprhdl.PexprScalarRepChild(2 /*child_index*/);

	// join stats of the children
	gpos::Ref<IStatisticsArray> statistics_array =
		GPOS_NEW(mp) IStatisticsArray(mp);
	;
	statistics_array->Append(outer_stats);
	;
	statistics_array->Append(inner_side_stats);
	gpos::Ref<IStatistics> stats = CJoinStatsProcessor::CalcAllJoinStats(
		mp, statistics_array.get(), pexprScalar,
		const_cast<CLogicalIndexApply *>(this));
	;

	return stats;
}

// return a copy of the operator with remapped columns
gpos::Ref<COperator>
CLogicalIndexApply::PopCopyWithRemappedColumns(CMemoryPool *mp,
											   UlongToColRefMap *colref_mapping,
											   BOOL must_exist)
{
	gpos::Ref<COperator> result = nullptr;
	gpos::Ref<CColRefArray> colref_array = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrOuterRefs.get(), colref_mapping, must_exist);
	gpos::Ref<CExpression> remapped_orig_join_pred = nullptr;

	if (nullptr != m_origJoinPred)
	{
		remapped_orig_join_pred = m_origJoinPred->PexprCopyWithRemappedColumns(
			mp, colref_mapping, must_exist);
	}

	result = GPOS_NEW(mp) CLogicalIndexApply(
		mp, std::move(colref_array), m_fOuterJoin, remapped_orig_join_pred);
	;

	return result;
}

// EOF
