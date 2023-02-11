//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLeftAntiSemiJoinStatsProcessor.h
//
//	@doc:
//		Processor for computing statistics for Left Anti-Semi Join
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CLeftAntiSemiJoinStatsProcessor_H
#define GPNAUCRATES_CLeftAntiSemiJoinStatsProcessor_H

#include "gpos/common/owner.h"

#include "naucrates/statistics/CJoinStatsProcessor.h"



namespace gpnaucrates
{
class CLeftAntiSemiJoinStatsProcessor : public CJoinStatsProcessor
{
public:
	// helper for LAS-joining histograms
	static void JoinHistogramsLASJ(
		const gpos::Ref<CHistogram> histogram1,
		const gpos::Ref<CHistogram> histogram2,
		gpos::Ref<CStatsPredJoin> join_stats, CDouble num_rows1,
		CDouble num_rows2,
		gpos::Ref<CHistogram> *result_hist1,  // output: histogram 1 after join
		gpos::Ref<CHistogram> *result_hist2,  // output: histogram 2 after join
		CDouble *scale_factor,	// output: scale factor based on the join
		BOOL is_input_empty,	// if true, one of the inputs is empty
		IStatistics::EStatsJoinType join_type,
		BOOL DoIgnoreLASJHistComputation);
	// left anti semi join with another stats structure
	static gpos::Ref<CStatistics> CalcLASJoinStatsStatic(
		CMemoryPool *mp, const gpos::Ref<IStatistics> outer_stats_input,
		const gpos::Ref<IStatistics> inner_stats_input,
		CStatsPredJoinArray *join_preds_stats,
		BOOL
			DoIgnoreLASJHistComputation	 // except for the case of LOJ cardinality estimation this flag is always
		// "true" since LASJ stats computation is very aggressive
	);
	// compute the null frequency for LASJ
	static CDouble NullFreqLASJ(CStatsPred::EStatsCmpType stats_cmp_type,
								const gpos::Ref<CHistogram> outer_histogram,
								const gpos::Ref<CHistogram> inner_histogram);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CLeftAntiSemiJoinStatsProcessor_H

// EOF
