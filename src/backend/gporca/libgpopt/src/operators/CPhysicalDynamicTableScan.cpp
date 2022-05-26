//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalDynamicTableScan.cpp
//
//	@doc:
//		Implementation of dynamic table scan operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalDynamicTableScan.h"

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/CPartIndexMap.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/exception.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicTableScan::CPhysicalDynamicTableScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalDynamicTableScan::CPhysicalDynamicTableScan(
	CMemoryPool *mp, BOOL is_partial, const CName *pnameAlias,
	CTableDescriptor *ptabdesc, ULONG ulOriginOpId, ULONG scan_id,
	CColRefArray *pdrgpcrOutput, CColRef2dArray *pdrgpdrgpcrParts,
	ULONG ulSecondaryScanId, CPartConstraint *ppartcnstr,
	CPartConstraint *ppartcnstrRel)
	: CPhysicalDynamicScan(mp, is_partial, ptabdesc, ulOriginOpId, pnameAlias,
						   scan_id, pdrgpcrOutput, pdrgpdrgpcrParts,
						   ulSecondaryScanId, ppartcnstr, ppartcnstrRel)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicTableScan::Matches
//
//	@doc:
//		match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalDynamicTableScan::Matches(COperator *pop) const
{
	return CUtils::FMatchDynamicScan(this, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicTableScan::PstatsDerive
//
//	@doc:
//		Statistics derivation during costing
//
//---------------------------------------------------------------------------
IStatistics *
CPhysicalDynamicTableScan::PstatsDerive(CMemoryPool *,
										CExpressionHandle &exprhdl,
										CReqdPropPlan *,
										IStatisticsArray *	// stats_ctxt
) const
{
	// extract part table base stats from passed handle
	IStatistics *base_table_stats = exprhdl.Pstats();
	GPOS_ASSERT(NULL != base_table_stats);
	if (NULL == base_table_stats)
	{
		GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiNoStats,
				   GPOS_WSZ_LIT("CPhysicalDynamicScan"));
	}

	base_table_stats->AddRef();
	return base_table_stats;
}

// EOF
