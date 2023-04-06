//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2023 VMware Inc.
//
//	@filename:
//		CPhysicalDynamicForeignScan.h
//
//	@doc:
//  	Foreign scan operator for multiple tables sharing a common
//  	column layout
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalDynamicForeignScan_H
#define GPOPT_CPhysicalDynamicForeignScan_H

#include "gpos/base.h"

#include "gpopt/operators/CPhysicalDynamicScan.h"

namespace gpopt
{
// Foreign scan operator for multiple tables sharing a common column layout.
class CPhysicalDynamicForeignScan : public CPhysicalDynamicScan
{
private:
	OID m_foreign_server_oid;

	BOOL m_is_master_only;

public:
	CPhysicalDynamicForeignScan(const CPhysicalDynamicForeignScan &) = delete;

	// ctor
	CPhysicalDynamicForeignScan(
		CMemoryPool *mp, const CName *pnameAlias,
		gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId, ULONG scan_id,
		CColRefArray *pdrgpcrOutput, CColRef2dArray *pdrgpdrgpcrParts,
		IMdIdArray *partition_mdids,
		ColRefToUlongMapArray *root_col_mapping_per_part,
		OID foreign_server_oid, BOOL is_master_only);


	EOperatorId
	Eopid() const override
	{
		return EopPhysicalDynamicForeignScan;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalDynamicForeignScan";
	}

	// match function
	BOOL Matches(gpos::Ref<COperator>) const override;

	// statistics derivation during costing
	gpos::Ref<IStatistics> PstatsDerive(CMemoryPool *,			   // mp
										CExpressionHandle &,	   // exprhdl
										gpos::Ref<CReqdPropPlan>,  // prpplan
										IStatisticsArray *		   //stats_ctxt
	) const override;

	OID
	GetForeignServerOid() const
	{
		return m_foreign_server_oid;
	}

	BOOL
	IsMasterOnly() const
	{
		return m_is_master_only;
	}
	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive rewindability
	gpos::Ref<CRewindabilitySpec>
	PrsDerive(CMemoryPool *mp,
			  CExpressionHandle &  // exprhdl
	) const override
	{
		// foreign tables are neither rewindable nor rescannable
		return GPOS_NEW(mp) CRewindabilitySpec(
			CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);
	}

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &exprhdl,
		const gpos::Ref<CEnfdRewindability> per) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CPhysicalDynamicForeignScan>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalDynamicForeignScan == pop->Eopid());

		return reinterpret_cast<gpos::Ref<CPhysicalDynamicForeignScan> >(pop);
	}

	gpos::Ref<CPartitionPropagationSpec> PppsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

};	// class CPhysicalDynamicForeignScan

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalDynamicForeignScan_H

// EOF
