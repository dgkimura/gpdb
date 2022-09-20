//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CUnionAllStatsProcessor.h
//
//	@doc:
//		Compute statistics for union all operation
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CUnionAllStatsProcessor_H
#define GPNAUCRATES_CUnionAllStatsProcessor_H

#include "gpos/common/owner.h"

#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/statistics/CStatistics.h"

namespace gpnaucrates
{
class CUnionAllStatsProcessor
{
public:
	static CStatistics *CreateStatsForUnionAll(
		CMemoryPool *mp, gpos::pointer<const CStatistics *> stats_first_child,
		gpos::pointer<const CStatistics *> stats_second_child,
		gpos::owner<ULongPtrArray *> output_colids,
		gpos::owner<ULongPtrArray *> first_child_colids,
		gpos::owner<ULongPtrArray *> second_child_colids);
};
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CUnionAllStatsProcessor_H

// EOF
