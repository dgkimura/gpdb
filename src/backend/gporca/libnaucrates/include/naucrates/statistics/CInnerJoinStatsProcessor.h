//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CInnerJoinStatsProcessor.h
//
//	@doc:
//		Processor for computing statistics for Inner Join
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CInnerJoinStatsProcessor_H
#define GPNAUCRATES_CInnerJoinStatsProcessor_H

#include "naucrates/statistics/CJoinStatsProcessor.h"


namespace gpnaucrates
{
class CInnerJoinStatsProcessor : public CJoinStatsProcessor
{
public:
	// inner join with another stats structure
	static gpos::Ref<CStatistics> CalcInnerJoinStatsStatic(
		CMemoryPool *mp, const gpos::Ref<IStatistics> outer_stats_input,
		const gpos::Ref<IStatistics> inner_stats_input,
		gpos::Ref<CStatsPredJoinArray> join_preds_stats);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CInnerJoinStatsProcessor_H

// EOF
