//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLeftSemiJoinStatsProcessor.cpp
//
//	@doc:
//		Statistics helper routines for processing Left Semi Joins
//---------------------------------------------------------------------------

#include "naucrates/statistics/CLeftSemiJoinStatsProcessor.h"

#include "gpopt/search/CGroupExpression.h"
#include "naucrates/statistics/CGroupByStatsProcessor.h"

using namespace gpopt;

// return statistics object after performing LSJ operation
gpos::Ref<CStatistics>
CLeftSemiJoinStatsProcessor::CalcLSJoinStatsStatic(
	CMemoryPool *mp, const gpos::Ref<IStatistics> outer_stats_input,
	const gpos::Ref<IStatistics> inner_stats_input,
	gpos::Ref<CStatsPredJoinArray> join_preds_stats)
{
	GPOS_ASSERT(nullptr != outer_stats_input);
	GPOS_ASSERT(nullptr != inner_stats_input);
	GPOS_ASSERT(nullptr != join_preds_stats);

	const ULONG length = join_preds_stats->Size();

	// iterate over all inner columns and perform a group by to remove duplicates
	gpos::Ref<ULongPtrArray> inner_colids = GPOS_NEW(mp) ULongPtrArray(mp);
	for (ULONG ul = 0; ul < length; ul++)
	{
		if ((*join_preds_stats)[ul]->HasValidColIdInner())
		{
			ULONG colid = ((*join_preds_stats)[ul])->ColIdInner();
			inner_colids->Append(GPOS_NEW(mp) ULONG(colid));
		}
	}

	// dummy agg columns required for group by derivation
	gpos::Ref<ULongPtrArray> aggs = GPOS_NEW(mp) ULongPtrArray(mp);
	gpos::Ref<IStatistics> inner_stats =
		CGroupByStatsProcessor::CalcGroupByStats(
			mp, gpos::dyn_cast<CStatistics>(inner_stats_input), inner_colids,
			aggs,
			nullptr	 // keys: no keys, use all grouping cols
		);

	const gpos::Ref<CStatistics> outer_stats =
		gpos::dyn_cast<CStatistics>(outer_stats_input);
	gpos::Ref<CStatistics> semi_join_stats =
		CJoinStatsProcessor::SetResultingJoinStats(
			mp, outer_stats->GetStatsConfig(), outer_stats, inner_stats,
			join_preds_stats, IStatistics::EsjtLeftSemiJoin /* esjt */,
			true /* DoIgnoreLASJHistComputation */
		);

	// clean up



	return semi_join_stats;
}

// EOF
