//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp
//
//	@filename:
//		CPhysicalDynamicIndexScan.h
//
//	@doc:
//		Physical dynamic index scan operators on partitioned tables
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalDynamicIndexScan_H
#define GPOPT_CPhysicalDynamicIndexScan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/metadata/CIndexDescriptor.h"
#include "gpopt/operators/CPhysicalDynamicScan.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;
class CIndexDescriptor;
class CName;
class CPartConstraint;

//---------------------------------------------------------------------------
//	@class:
//		CPhysicalDynamicIndexScan
//
//	@doc:
//		Physical dynamic index scan operators for partitioned tables
//
//---------------------------------------------------------------------------
class CPhysicalDynamicIndexScan : public CPhysicalDynamicScan
{
private:
	// index descriptor
	gpos::Ref<CIndexDescriptor> m_pindexdesc;

	// order
	gpos::Ref<COrderSpec> m_pos;

public:
	CPhysicalDynamicIndexScan(const CPhysicalDynamicIndexScan &) = delete;

	// ctors
	CPhysicalDynamicIndexScan(
		CMemoryPool *mp, gpos::Ref<CIndexDescriptor> pindexdesc,
		gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId,
		const CName *pnameAlias, gpos::Ref<CColRefArray> pdrgpcrOutput,
		ULONG scan_id, gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart,
		gpos::Ref<COrderSpec> pos, gpos::Ref<IMdIdArray> partition_mdids,
		gpos::Ref<ColRefToUlongMapArray> root_col_mapping_per_part);

	// dtor
	~CPhysicalDynamicIndexScan() override;


	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalDynamicIndexScan;
	}

	// operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalDynamicIndexScan";
	}

	// index descriptor
	CIndexDescriptor *
	Pindexdesc() const
	{
		return m_pindexdesc.get();
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(COperator *pop) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::Ref<COrderSpec>
	PosDerive(CMemoryPool *,	   //mp
			  CExpressionHandle &  //exprhdl
	) const override
	{
		;
		return m_pos;
	}

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl, const CEnfdOrder *peo) const override;

	// conversion function
	static CPhysicalDynamicIndexScan *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalDynamicIndexScan == pop->Eopid());

		return dynamic_cast<CPhysicalDynamicIndexScan *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

	// statistics derivation during costing
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl, CReqdPropPlan *prpplan,
		IStatisticsArray *stats_ctxt) const override;

};	// class CPhysicalDynamicIndexScan

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalDynamicIndexScan_H

// EOF
