//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2020-Present VMware, Inc. or its affiliates
//---------------------------------------------------------------------------

#ifndef GPOPT_CPartSelectorInfo_H
#define GPOPT_CPartSelectorInfo_H

#include <gpopt/operators/CExpression.h>
#include <naucrates/statistics/IStatistics.h>

#include "gpos/common/owner.h"

namespace gpopt
{
struct SPartSelectorInfoEntry
{
	// selector id
	ULONG m_selector_id;

	// filter stored in the partition selector
	gpos::owner<CExpression *> m_filter_expr;

	// statistics of the subtree of the partition selector
	gpos::owner<IStatistics *> m_stats;

	SPartSelectorInfoEntry(ULONG mSelectorId,
						   gpos::owner<CExpression *> mFilterExpr,
						   gpos::owner<IStatistics *> mStats)
		: m_selector_id(mSelectorId),
		  m_filter_expr(std::move(mFilterExpr)),
		  m_stats(std::move(mStats))
	{
	}

	~SPartSelectorInfoEntry()
	{
		m_filter_expr->Release();
		m_stats->Release();
	}
};

using SPartSelectorInfo =
	CHashMap<ULONG, SPartSelectorInfoEntry, gpos::HashValue<ULONG>,
			 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
			 CleanupDelete<SPartSelectorInfoEntry>>;

}  // namespace gpopt
#endif	// !GPOPT_CPartSelectorInfo_H

// EOF
