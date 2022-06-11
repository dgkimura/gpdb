//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalExternalScan.cpp
//
//	@doc:
//		Implementation of external scan operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalExternalScan.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalExternalScan::CPhysicalExternalScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalExternalScan::CPhysicalExternalScan(
	CMemoryPool *mp, const CName *pnameAlias,
	gpos::owner<CTableDescriptor *> ptabdesc, CColRefArray *pdrgpcrOutput)
	: CPhysicalTableScan(mp, pnameAlias, ptabdesc, pdrgpcrOutput)
{
	// if this table is master only, then keep the original distribution spec.
	if (IMDRelation::EreldistrMasterOnly == ptabdesc->GetRelDistribution())
	{
		return;
	}

	// otherwise, override the distribution spec for external table
	if (m_pds)
	{
		m_pds->Release();
	}

	m_pds = GPOS_NEW(mp) CDistributionSpecRandom();
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalExternalScan::Matches
//
//	@doc:
//		match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalExternalScan::Matches(gpos::pointer<COperator *> pop) const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	gpos::pointer<CPhysicalExternalScan *> popExternalScan =
		gpos::dyn_cast<CPhysicalExternalScan>(pop);
	return m_ptabdesc == popExternalScan->Ptabdesc() &&
		   m_pdrgpcrOutput->Equals(popExternalScan->PdrgpcrOutput());
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalExternalScan::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalExternalScan::EpetRewindability(
	CExpressionHandle &exprhdl,
	gpos::pointer<const CEnfdRewindability *> per) const
{
	gpos::pointer<CRewindabilitySpec *> prs =
		gpos::dyn_cast<CDrvdPropPlan>(exprhdl.Pdp())->Prs();
	if (per->FCompatible(prs))
	{
		return CEnfdProp::EpetUnnecessary;
	}

	return CEnfdProp::EpetRequired;
}

// EOF
