//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLeftSemiJoinStatsProcessor.h
//
//	@doc:
//		Processor for computing statistics for Left Semi Join
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CLeftSemiJoinStatsProcessor_H
#define GPNAUCRATES_CLeftSemiJoinStatsProcessor_H

#include "gpos/common/owner.h"

#include "naucrates/statistics/CJoinStatsProcessor.h"

namespace gpnaucrates
{
class CLeftSemiJoinStatsProcessor : public CJoinStatsProcessor
{
public:
	static gpos::Ref<CStatistics> CalcLSJoinStatsStatic(
		CMemoryPool *mp, const IStatistics *outer_stats,
		const IStatistics *inner_side_stats,
		CStatsPredJoinArray *join_preds_stats);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CLeftSemiJoinStatsProcessor_H

// EOF
