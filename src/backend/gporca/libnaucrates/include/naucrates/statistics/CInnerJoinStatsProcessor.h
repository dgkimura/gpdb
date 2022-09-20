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

#include "gpos/common/owner.h"

#include "naucrates/statistics/CJoinStatsProcessor.h"


namespace gpnaucrates
{
class CInnerJoinStatsProcessor : public CJoinStatsProcessor
{
public:
	// inner join with another stats structure
	static CStatistics *CalcInnerJoinStatsStatic(
		CMemoryPool *mp, gpos::pointer<const IStatistics *> outer_stats_input,
		gpos::pointer<const IStatistics *> inner_stats_input,
		gpos::pointer<CStatsPredJoinArray *> join_preds_stats);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CInnerJoinStatsProcessor_H

// EOF
