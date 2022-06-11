//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLeftOuterJoinStatsProcessor.h
//
//	@doc:
//		Processor for computing statistics for Left Outer Join
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CLeftOuterJoinStatsProcessor_H
#define GPNAUCRATES_CLeftOuterJoinStatsProcessor_H

#include "gpos/common/owner.h"

#include "naucrates/statistics/CJoinStatsProcessor.h"

namespace gpnaucrates
{
class CLeftOuterJoinStatsProcessor : public CJoinStatsProcessor
{
private:
	// create a new hash map of histograms from the results of the inner join and the histograms of the outer child
	static gpos::owner<UlongToHistogramMap *> MakeLOJHistogram(
		CMemoryPool *mp, gpos::pointer<const CStatistics *> outer_stats,
		gpos::pointer<const CStatistics *> inner_side_stats,
		gpos::pointer<CStatistics *> inner_join_stats,
		gpos::pointer<CStatsPredJoinArray *> join_preds_stats,
		CDouble num_rows_inner_join, CDouble *result_rows_LASJ);
	// helper method to add histograms of the inner side of a LOJ
	static void AddHistogramsLOJInner(
		CMemoryPool *mp, gpos::pointer<const CStatistics *> inner_join_stats,
		gpos::pointer<ULongPtrArray *> inner_colids_with_stats,
		CDouble num_rows_LASJ, CDouble num_rows_inner_join,
		gpos::pointer<UlongToHistogramMap *> LOJ_histograms);

public:
	static gpos::owner<CStatistics *> CalcLOJoinStatsStatic(
		CMemoryPool *mp, gpos::pointer<const IStatistics *> outer_stats,
		gpos::pointer<const IStatistics *> inner_side_stats,
		gpos::pointer<CStatsPredJoinArray *> join_preds_stats);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CLeftOuterJoinStatsProcessor_H

// EOF
