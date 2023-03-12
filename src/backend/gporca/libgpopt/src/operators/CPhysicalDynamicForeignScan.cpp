//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2023 VMware Inc.
//
//	@filename:
//		CPhysicalDynamicForeignScan.cpp
//
//	@doc:
//		Implementation of dynamic foreign scan operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalDynamicForeignScan.h"

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/CDistributionSpecStrictSingleton.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicForeignScan::CPhysicalDynamicForeignScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalDynamicForeignScan::CPhysicalDynamicForeignScan(
	CMemoryPool *mp, const CName *pnameAlias,
	gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId, ULONG scan_id,
	gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrParts,
	gpos::Ref<IMdIdArray> partition_mdids,
	gpos::Ref<ColRefToUlongMapArray> root_col_mapping_per_part,
	OID foreign_server_oid, BOOL is_master_only)


	: CPhysicalDynamicScan(mp, ptabdesc, ulOriginOpId, pnameAlias, scan_id,
						   pdrgpcrOutput, pdrgpdrgpcrParts, partition_mdids,
						   root_col_mapping_per_part),
	  m_foreign_server_oid(foreign_server_oid),
	  m_is_master_only(is_master_only)
{
	// if this table is master only, use a strict singleton distribution request
	if (is_master_only)
	{
		m_pds = GPOS_NEW(mp) CDistributionSpecStrictSingleton(
			CDistributionSpecSingleton::EstMaster);
	}
	// otherwise, we want to execute on each segment (but can't assume anything about the distribution)
	else
	{
		m_pds = GPOS_NEW(mp) CDistributionSpecRandom();
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicForeignScan::Matches
//
//	@doc:
//		match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalDynamicForeignScan::Matches(gpos::Ref<COperator> pop) const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	gpos::Ref<CPhysicalDynamicForeignScan> popForeignScan =
		CPhysicalDynamicForeignScan::PopConvert(pop);
	return CUtils::FMatchDynamicScan(
			   gpos::dyn_cast<CPhysicalDynamicForeignScan>(SharedFromThis()),
			   pop) &&
		   m_foreign_server_oid == popForeignScan->GetForeignServerOid();
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicForeignScan::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalDynamicForeignScan::EpetRewindability(
	CExpressionHandle &exprhdl, const gpos::Ref<CEnfdRewindability> per) const
{
	gpos::Ref<CRewindabilitySpec> prs =
		CDrvdPropPlan::Pdpplan(exprhdl.Pdp())->Prs();
	if (per->FCompatible(prs))
	{
		return CEnfdProp::EpetUnnecessary;
	}

	return CEnfdProp::EpetRequired;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicForeignScan::PstatsDerive
//
//	@doc:
//		Statistics derivation during costing
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CPhysicalDynamicForeignScan::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::Ref<CReqdPropPlan> prpplan,
	gpos::Ref<IStatisticsArray>	 // stats_ctxt
) const
{
	GPOS_ASSERT(nullptr != prpplan);

	return CStatisticsUtils::DeriveStatsForDynamicScan(
		mp, exprhdl, ScanId(), prpplan->Pepp()->PppsRequired());
}

gpos::Ref<CPartitionPropagationSpec>
CPhysicalDynamicForeignScan::PppsDerive(CMemoryPool *mp,
										CExpressionHandle &) const
{
	gpos::Ref<CPartitionPropagationSpec> pps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	pps->Insert(ScanId(), CPartitionPropagationSpec::EpptConsumer,
				Ptabdesc()->MDId(), nullptr, nullptr);

	return pps;
}


// EOF
