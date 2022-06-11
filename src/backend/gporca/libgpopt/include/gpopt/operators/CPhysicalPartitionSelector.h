//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalPartitionSelector.h
//
//	@doc:
//		Physical partition selector operator used for property enforcement
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalPartitionSelector_H
#define GPOPT_CPhysicalPartitionSelector_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/operators/CPhysical.h"


namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalPartitionSelector
//
//	@doc:
//		Physical partition selector operator used for property enforcement
//
//---------------------------------------------------------------------------
class CPhysicalPartitionSelector : public CPhysical
{
private:
	// Scan id
	ULONG m_scan_id;

	// Unique id per Partition Selector created
	ULONG m_selector_id;

	// mdid of partitioned table
	gpos::owner<IMDId *> m_mdid;

	// partition selection predicate
	gpos::owner<CExpression *> m_filter_expr;

public:
	CPhysicalPartitionSelector(const CPhysicalPartitionSelector &) = delete;

	// ctor
	CPhysicalPartitionSelector(CMemoryPool *mp, ULONG scan_id,
							   ULONG selector_id, gpos::owner<IMDId *> mdid,
							   gpos::owner<CExpression *> pexprScalar);

	// dtor
	~CPhysicalPartitionSelector() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalPartitionSelector;
	}

	// operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalPartitionSelector";
	}

	// scan id
	ULONG
	ScanId() const
	{
		return m_scan_id;
	}

	ULONG
	SelectorId() const
	{
		return m_selector_id;
	}

	// partitioned table mdid
	gpos::pointer<IMDId *>
	MDId() const
	{
		return m_mdid;
	}

	// return the partition selection predicate
	gpos::pointer<CExpression *>
	FilterExpr() const
	{
		return m_filter_expr;
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// hash function
	ULONG HashValue() const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		// operator has one child
		return false;
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	gpos::owner<CColRefSet *> PcrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt, ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	gpos::owner<CCTEReq *> PcteRequired(CMemoryPool *mp,
										CExpressionHandle &exprhdl,
										gpos::pointer<CCTEReq *> pcter,
										ULONG child_index,
										CDrvdPropArray *pdrgpdpCtxt,
										ULONG ulOptReq) const override;

	// compute required sort order of the n-th child
	gpos::owner<COrderSpec *> PosRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<COrderSpec *> posRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required distribution of the n-th child
	gpos::owner<CDistributionSpec *> PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CDistributionSpec *> pdsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::owner<CRewindabilitySpec *> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CRewindabilitySpec *> prsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	gpos::owner<CPartitionPropagationSpec *> PppsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CPartitionPropagationSpec *> prsRequired,
		ULONG child_index, gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl,
						   gpos::pointer<CColRefSet *> pcrsRequired,
						   ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::owner<COrderSpec *> PosDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive distribution
	gpos::owner<CDistributionSpec *> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::owner<CRewindabilitySpec *> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	gpos::owner<CPartitionPropagationSpec *> PppsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return distribution property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetDistribution(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdDistribution *> ped) const override;

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdRewindability *> per) const override;

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	// return true if operator passes through stats obtained from children,
	// this is used when computing stats during costing
	BOOL
	FPassThruStats() const override
	{
		return true;
	}

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CPhysicalPartitionSelector *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalPartitionSelector == pop->Eopid());

		return dynamic_cast<CPhysicalPartitionSelector *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &os) const override;

};	// class CPhysicalPartitionSelector

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalPartitionSelector_H

// EOF
