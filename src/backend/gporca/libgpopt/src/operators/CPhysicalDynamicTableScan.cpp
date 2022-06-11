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
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/CUtils.h"
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
	CMemoryPool *mp, const CName *pnameAlias,
	gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId, ULONG scan_id,
	gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrParts,
	gpos::Ref<IMdIdArray> partition_mdids,
	gpos::Ref<ColRefToUlongMapArray> root_col_mapping_per_part)
	: CPhysicalDynamicScan(
		  mp, std::move(ptabdesc), ulOriginOpId, pnameAlias, scan_id,
		  std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrParts),
		  std::move(partition_mdids), std::move(root_col_mapping_per_part))
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
gpos::Ref<IStatistics>
CPhysicalDynamicTableScan::PstatsDerive(CMemoryPool *mp,
										CExpressionHandle &exprhdl,
										CReqdPropPlan *prpplan,
										IStatisticsArray *	// stats_ctxt
) const
{
	GPOS_ASSERT(nullptr != prpplan);

	return CStatisticsUtils::DeriveStatsForDynamicScan(
		mp, exprhdl, ScanId(), prpplan->Pepp()->PppsRequired());
}


gpos::Ref<CPartitionPropagationSpec>
CPhysicalDynamicTableScan::PppsDerive(CMemoryPool *mp,
									  CExpressionHandle &) const
{
	gpos::Ref<CPartitionPropagationSpec> pps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	pps->Insert(ScanId(), CPartitionPropagationSpec::EpptConsumer,
				Ptabdesc()->MDId(), nullptr, nullptr);

	return pps;
}

// EOF
