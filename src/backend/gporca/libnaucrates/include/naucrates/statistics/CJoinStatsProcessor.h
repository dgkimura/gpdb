//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CJoinStatsProcessor.h
//
//	@doc:
//		Compute statistics for all joins
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CJoinStatsProcessor_H
#define GPNAUCRATES_CJoinStatsProcessor_H

#include "gpos/common/owner.h"

#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/statistics/CGroupByStatsProcessor.h"
#include "naucrates/statistics/CScaleFactorUtils.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"

namespace gpnaucrates
{
// Parent class for computing statistics for all joins
class CJoinStatsProcessor
{
	static BOOL m_compute_scale_factor_from_histogram_buckets;

protected:
	// return join cardinality based on scaling factor and join type
	static CDouble CalcJoinCardinality(
		CMemoryPool *mp, gpos::pointer<CStatisticsConfig *> stats_config,
		CDouble left_num_rows, CDouble right_num_rows,
		gpos::pointer<CScaleFactorUtils::SJoinConditionArray *>
			join_conds_scale_factors,
		IStatistics::EStatsJoinType join_type);


	// check if the join statistics object is empty output based on the input
	// histograms and the join histograms
	static BOOL JoinStatsAreEmpty(BOOL outer_is_empty, BOOL output_is_empty,
								  const CHistogram *outer_histogram,
								  const CHistogram *inner_histogram,
								  CHistogram *join_histogram,
								  IStatistics::EStatsJoinType join_type);

	// helper for joining histograms
	static void JoinHistograms(
		CMemoryPool *mp, const CHistogram *histogram1,
		const CHistogram *histogram2,
		gpos::pointer<CStatsPredJoin *> join_pred_stats, CDouble num_rows1,
		CDouble num_rows2,
		CHistogram **result_hist1,	// output: histogram 1 after join
		CHistogram **result_hist2,	// output: histogram 2 after join
		CDouble *scale_factor,		// output: scale factor based on the join
		BOOL is_input_empty,		// if true, one of the inputs is empty
		IStatistics::EStatsJoinType join_type,
		BOOL DoIgnoreLASJHistComputation);

public:
	// main driver to generate join stats
	static gpos::owner<CStatistics *> SetResultingJoinStats(
		CMemoryPool *mp, gpos::pointer<CStatisticsConfig *> stats_config,
		gpos::pointer<const IStatistics *> outer_stats_input,
		gpos::pointer<const IStatistics *> inner_stats_input,
		gpos::pointer<CStatsPredJoinArray *> join_preds_stats,
		IStatistics::EStatsJoinType join_type,
		BOOL DoIgnoreLASJHistComputation);

	static gpos::owner<IStatistics *> CalcAllJoinStats(
		CMemoryPool *mp, gpos::pointer<IStatisticsArray *> statistics_array,
		CExpression *expr, gpos::pointer<COperator *> pop);

	// derive statistics for join operation given array of statistics object
	static gpos::owner<IStatistics *> DeriveJoinStats(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt);

	// derive statistics when scalar expression has outer references
	static gpos::owner<IStatistics *> DeriveStatsWithOuterRefs(
		CMemoryPool *mp,
		CExpressionHandle &
			exprhdl,  // handle attached to the logical expression we want to derive stats for
		CExpression *expr,	 // scalar condition used for stats derivation
		IStatistics *stats,	 // statistics object of attached expression
		gpos::pointer<IStatisticsArray *>
			all_outer_stats	 // array of stats objects where outer references are defined
	);

	static void
	SetComputeScaleFactorFromHistogramBuckets(BOOL val)
	{
		m_compute_scale_factor_from_histogram_buckets = val;
	}

	static BOOL
	ComputeScaleFactorFromHistogramBuckets()
	{
		return m_compute_scale_factor_from_histogram_buckets;
	}
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CJoinStatsProcessor_H

// EOF
