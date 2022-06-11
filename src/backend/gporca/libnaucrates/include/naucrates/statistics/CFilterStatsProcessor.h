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
#include "gpos/common/owner.h"

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
	static CHistogram *MakeHistSimpleFilter(
		CMemoryPool *mp, gpos::pointer<CStatsPred *> pred_stats,
		gpos::pointer<CBitSet *> filter_colids, CHistogram *hist_before,
		CDouble *last_scale_factor, ULONG *target_last_colid);

	// create a new histogram after applying a point filter
	static CHistogram *MakeHistPointFilter(
		gpos::pointer<CStatsPredPoint *> pred_stats,
		gpos::pointer<CBitSet *> filter_colids, CHistogram *hist_before,
		CDouble *last_scale_factor, ULONG *target_last_colid);

	// create a new histogram after applying a LIKE filter
	static CHistogram *MakeHistLikeFilter(
		gpos::pointer<CStatsPredLike *> pred_stats,
		gpos::pointer<CBitSet *> filter_colids, CHistogram *hist_before,
		CDouble *last_scale_factor, ULONG *target_last_colid);

	// create a new histogram for an unsupported predicate
	static CHistogram *MakeHistUnsupportedPred(
		gpos::pointer<CStatsPredUnsupported *> pred_stats,
		gpos::pointer<CBitSet *> filter_colids, CHistogram *hist_before,
		CDouble *last_scale_factor, ULONG *target_last_colid);

	// create a new histogram after applying a pred op ANY(ARRAY[...]) filter
	static CHistogram *MakeHistArrayCmpAnyFilter(
		CMemoryPool *mp, gpos::pointer<CStatsPredArrayCmp *> pred_stats,
		gpos::pointer<CBitSet *> filter_colids, CHistogram *hist_before,
		CDouble *last_scale_factor, ULONG *target_last_colid);

	// create a new hash map of histograms after applying a conjunctive or disjunctive filter
	static gpos::owner<UlongToHistogramMap *> MakeHistHashMapConjOrDisjFilter(
		CMemoryPool *mp, gpos::pointer<const CStatisticsConfig *> stats_config,
		UlongToHistogramMap *input_histograms, CDouble input_rows,
		gpos::pointer<CStatsPred *> pred_stats, CDouble *scale_factor);

	// create new hash map of histograms after applying the conjunction predicate
	static gpos::owner<UlongToHistogramMap *> MakeHistHashMapConjFilter(
		CMemoryPool *mp, gpos::pointer<const CStatisticsConfig *> stats_config,
		gpos::pointer<UlongToHistogramMap *> intermediate_histograms,
		CDouble input_rows,
		gpos::pointer<CStatsPredConj *> conjunctive_pred_stats,
		CDouble *scale_factor);

	// create new hash map of histograms after applying the disjunctive predicate
	static gpos::owner<UlongToHistogramMap *> MakeHistHashMapDisjFilter(
		CMemoryPool *mp, gpos::pointer<const CStatisticsConfig *> stats_config,
		UlongToHistogramMap *input_histograms, CDouble input_rows,
		gpos::pointer<CStatsPredDisj *> pred_stats, CDouble *scale_factor);

	// check if the column is a new column for statistic calculation
	static BOOL IsNewStatsColumn(ULONG colid, ULONG last_colid);

public:
	// filter
	static gpos::owner<CStatistics *> MakeStatsFilter(
		CMemoryPool *mp, gpos::pointer<const CStatistics *> input_stats,
		gpos::pointer<CStatsPred *> base_pred_stats, BOOL do_cap_NDVs);

	// derive statistics for filter operation based on given scalar expression
	static gpos::pointer<IStatistics *> MakeStatsFilterForScalarExpr(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatistics *> child_stats,
		gpos::pointer<CExpression *>
			local_scalar_expr,	// filter expression on local columns only
		CExpression *
			outer_refs_scalar_expr,	 // filter expression involving outer references
		gpos::pointer<IStatisticsArray *> all_outer_stats);

	static CDouble SelectivityOfPredicate(
		CMemoryPool *mp, CExpression *pred,
		gpos::pointer<CTableDescriptor *> ptabdesc, CColRefSet *pcrsOuterRefs);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CFilterStatsProcessor_H

// EOF
