//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CFilterStatsProcessor.h
//
//	@doc:
//		Compute statistics for filter operation
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CFilterStatsProcessor_H
#define GPNAUCRATES_CFilterStatsProcessor_H

#include "gpos/base.h"

#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/statistics/CJoinStatsProcessor.h"
#include "naucrates/statistics/CScaleFactorUtils.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"

namespace gpnaucrates
{
class CFilterStatsProcessor
{
private:
	// create a new histogram after applying the filter that is not an AND/OR predicate
	static CHistogram *MakeHistSimpleFilter(CMemoryPool *mp,
											gpos::Ref<CStatsPred> pred_stats,
											gpos::Ref<CBitSet> filter_colids,
											CHistogram *hist_before,
											CDouble *last_scale_factor,
											ULONG *target_last_colid);

	// create a new histogram after applying a point filter
	static CHistogram *MakeHistPointFilter(
		gpos::Ref<CStatsPredPoint> pred_stats, gpos::Ref<CBitSet> filter_colids,
		CHistogram *hist_before, CDouble *last_scale_factor,
		ULONG *target_last_colid);

	// create a new histogram after applying a LIKE filter
	static CHistogram *MakeHistLikeFilter(gpos::Ref<CStatsPredLike> pred_stats,
										  gpos::Ref<CBitSet> filter_colids,
										  CHistogram *hist_before,
										  CDouble *last_scale_factor,
										  ULONG *target_last_colid);

	// create a new histogram for an unsupported predicate
	static CHistogram *MakeHistUnsupportedPred(
		gpos::Ref<CStatsPredUnsupported> pred_stats,
		gpos::Ref<CBitSet> filter_colids, CHistogram *hist_before,
		CDouble *last_scale_factor, ULONG *target_last_colid);

	// create a new histogram after applying a pred op ANY(ARRAY[...]) filter
	static CHistogram *MakeHistArrayCmpAnyFilter(
		CMemoryPool *mp, gpos::Ref<CStatsPredArrayCmp> pred_stats,
		gpos::Ref<CBitSet> filter_colids, CHistogram *hist_before,
		CDouble *last_scale_factor, ULONG *target_last_colid);

	// create a new hash map of histograms after applying a conjunctive or disjunctive filter
	static gpos::Ref<UlongToHistogramMap> MakeHistHashMapConjOrDisjFilter(
		CMemoryPool *mp, const gpos::Ref<CStatisticsConfig> stats_config,
		gpos::Ref<UlongToHistogramMap> input_histograms, CDouble input_rows,
		gpos::Ref<CStatsPred> pred_stats, CDouble *scale_factor,
		const gpos::Ref<CStatistics> input_stats);

	// create new hash map of histograms after applying the conjunction predicate
	static gpos::Ref<UlongToHistogramMap> MakeHistHashMapConjFilter(
		CMemoryPool *mp, const gpos::Ref<CStatisticsConfig> stats_config,
		gpos::Ref<UlongToHistogramMap> intermediate_histograms,
		CDouble input_rows, gpos::Ref<CStatsPredConj> conjunctive_pred_stats,
		CDouble *scale_factor, const gpos::Ref<CStatistics> input_stats);

	// create new hash map of histograms after applying the disjunctive predicate
	static gpos::Ref<UlongToHistogramMap> MakeHistHashMapDisjFilter(
		CMemoryPool *mp, const gpos::Ref<CStatisticsConfig> stats_config,
		gpos::Ref<UlongToHistogramMap> input_histograms, CDouble input_rows,
		gpos::Ref<CStatsPredDisj> pred_stats, CDouble *scale_factor,
		const gpos::Ref<CStatistics> input_stats);

	// check if the column is a new column for statistic calculation
	static BOOL IsNewStatsColumn(ULONG colid, ULONG last_colid);

public:
	// filter
	static gpos::Ref<CStatistics> MakeStatsFilter(
		CMemoryPool *mp, const gpos::Ref<CStatistics> input_stats,
		gpos::Ref<CStatsPred> base_pred_stats, BOOL do_cap_NDVs);

	// derive statistics for filter operation based on given scalar expression
	static gpos::Ref<IStatistics> MakeStatsFilterForScalarExpr(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<IStatistics> child_stats,
		gpos::Ref<CExpression>
			local_scalar_expr,	// filter expression on local columns only
		gpos::Ref<CExpression>
			outer_refs_scalar_expr,	 // filter expression involving outer references
		gpos::Ref<IStatisticsArray> all_outer_stats);

	static CDouble SelectivityOfPredicate(CMemoryPool *mp,
										  gpos::Ref<CExpression> pred,
										  gpos::Ref<CTableDescriptor> ptabdesc,
										  gpos::Ref<CColRefSet> pcrsOuterRefs);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CFilterStatsProcessor_H

// EOF
