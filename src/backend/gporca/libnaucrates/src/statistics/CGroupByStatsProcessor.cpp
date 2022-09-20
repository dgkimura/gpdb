//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CGroupByStatsProcessor.cpp
//
//	@doc:
//		Statistics helper routines for processing group by operations
//---------------------------------------------------------------------------

#include "naucrates/statistics/CGroupByStatsProcessor.h"

#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;

// return statistics object after Group by computation
gpos::Ref<CStatistics>
CGroupByStatsProcessor::CalcGroupByStats(CMemoryPool *mp,
										 const CStatistics *input_stats,
										 ULongPtrArray *GCs,
										 ULongPtrArray *aggs, CBitSet *keys)
{
	// create hash map from colid -> histogram for resultant structure
	gpos::Ref<UlongToHistogramMap> col_histogram_mapping =
		GPOS_NEW(mp) UlongToHistogramMap(mp);

	// hash map colid -> width
	gpos::Ref<UlongToDoubleMap> col_width_mapping =
		GPOS_NEW(mp) UlongToDoubleMap(mp);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	gpos::Ref<CStatistics> agg_stats = nullptr;
	CDouble agg_rows = CStatistics::MinRows;
	if (input_stats->IsEmpty())
	{
		// add dummy histograms for the aggregates and grouping columns
		CHistogram::AddDummyHistogramAndWidthInfo(
			mp, col_factory, col_histogram_mapping.get(),
			col_width_mapping.get(), aggs, true /* is_empty */);
		CHistogram::AddDummyHistogramAndWidthInfo(
			mp, col_factory, col_histogram_mapping.get(),
			col_width_mapping.get(), GCs, true /* is_empty */);

		agg_stats = GPOS_NEW(mp)
			CStatistics(mp, col_histogram_mapping, col_width_mapping, agg_rows,
						true /* is_empty */);
	}
	else
	{
		// for computed aggregates, we're not going to be very smart right now
		CHistogram::AddDummyHistogramAndWidthInfo(
			mp, col_factory, col_histogram_mapping.get(),
			col_width_mapping.get(), aggs, false /* is_empty */);

		gpos::Ref<CColRefSet> computed_groupby_cols =
			GPOS_NEW(mp) CColRefSet(mp);
		gpos::Ref<CColRefSet> groupby_cols_for_stats =
			CStatisticsUtils::MakeGroupByColsForStats(
				mp, GCs, computed_groupby_cols.get());

		// add statistical information of columns (1) used to compute the cardinality of the aggregate
		// and (2) the grouping columns that are computed
		CStatisticsUtils::AddGrpColStats(
			mp, input_stats, groupby_cols_for_stats.get(),
			col_histogram_mapping.get(), col_width_mapping.get());
		CStatisticsUtils::AddGrpColStats(
			mp, input_stats, computed_groupby_cols.get(),
			col_histogram_mapping.get(), col_width_mapping.get());

		const CStatisticsConfig *stats_config = input_stats->GetStatsConfig();

		gpos::Ref<CDoubleArray> NDVs = CStatisticsUtils::ExtractNDVForGrpCols(
			mp, stats_config, input_stats, groupby_cols_for_stats.get(), keys);
		CDouble groups =
			CStatisticsUtils::GetCumulativeNDVs(stats_config, NDVs.get());

		// clean up
		;
		;
		;

		agg_rows = std::min(std::max(CStatistics::MinRows.Get(), groups.Get()),
							input_stats->Rows().Get());

		// create a new stats object for the output
		agg_stats = GPOS_NEW(mp)
			CStatistics(mp, col_histogram_mapping, col_width_mapping, agg_rows,
						input_stats->IsEmpty());
	}

	// In the output statistics object, the upper bound source cardinality of the grouping column
	// cannot be greater than the upper bound source cardinality information maintained in the input
	// statistics object. Therefore we choose CStatistics::EcbmMin the bounding method which takes
	// the minimum of the cardinality upper bound of the source column (in the input hash map)
	// and estimated group by cardinality.

	// modify source id to upper bound card information
	CStatisticsUtils::ComputeCardUpperBounds(
		mp, input_stats, agg_stats.get(), agg_rows,
		CStatistics::EcbmMin /* card_bounding_method */);

	return agg_stats;
}

// EOF
