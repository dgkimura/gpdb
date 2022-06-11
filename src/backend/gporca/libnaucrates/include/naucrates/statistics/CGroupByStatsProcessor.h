//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CGroupByStatsProcessor.h
//
//	@doc:
//		Compute statistics for group by operation
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CGroupByStatsProcessor_H
#define GPNAUCRATES_CGroupByStatsProcessor_H

#include "gpos/common/owner.h"

#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"

namespace gpnaucrates
{
class CGroupByStatsProcessor
{
public:
	// group by
	static gpos::owner<CStatistics *> CalcGroupByStats(
		CMemoryPool *mp, gpos::pointer<const CStatistics *> input_stats,
		gpos::pointer<ULongPtrArray *> GCs, gpos::pointer<ULongPtrArray *> aggs,
		gpos::pointer<CBitSet *> keys);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CGroupByStatsProcessor_H

// EOF
