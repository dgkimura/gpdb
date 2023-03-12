//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CInnerJoinStatsProcessor.cpp
//
//	@doc:
//		Statistics helper routines for processing Inner Joins
//---------------------------------------------------------------------------

#include "naucrates/statistics/CInnerJoinStatsProcessor.h"

#include "gpopt/search/CGroupExpression.h"
using namespace gpmd;

// return statistics object after performing inner join
gpos::Ref<CStatistics>
CInnerJoinStatsProcessor::CalcInnerJoinStatsStatic(
	CMemoryPool *mp, const gpos::Ref<IStatistics> outer_stats_input,
	const gpos::Ref<IStatistics> inner_stats_input,
	gpos::Ref<CStatsPredJoinArray> join_preds_stats)
{
	GPOS_ASSERT(nullptr != outer_stats_input);
	GPOS_ASSERT(nullptr != inner_stats_input);
	GPOS_ASSERT(nullptr != join_preds_stats);
	const gpos::Ref<CStatistics> outer_stats =
		gpos::dyn_cast<CStatistics>(outer_stats_input);

	return CJoinStatsProcessor::SetResultingJoinStats(
		mp, outer_stats->GetStatsConfig(), outer_stats_input, inner_stats_input,
		join_preds_stats, IStatistics::EsjtInnerJoin,
		true /* DoIgnoreLASJHistComputation */
	);
}

// EOF
