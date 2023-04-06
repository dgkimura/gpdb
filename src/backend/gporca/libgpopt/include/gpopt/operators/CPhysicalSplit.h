//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalSplit.h
//
//	@doc:
//		Physical split operator
//---------------------------------------------------------------------------
#ifndef GPOS_CPhysicalSplit_H
#define GPOS_CPhysicalSplit_H

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalSplit
//
//	@doc:
//		Physical split operator
//
//---------------------------------------------------------------------------
class CPhysicalSplit : public CPhysical
{
private:
	// deletion columns
	CColRefArray *m_pdrgpcrDelete;

	// insertion columns
	CColRefArray *m_pdrgpcrInsert;

	// ctid column
	gpos::Ref<CColRef> m_pcrCtid;

	// segmentid column
	gpos::Ref<CColRef> m_pcrSegmentId;

	// action column
	gpos::Ref<CColRef> m_pcrAction;

	// required columns by local members
	gpos::Ref<CColRefSet> m_pcrsRequiredLocal;

public:
	CPhysicalSplit(const CPhysicalSplit &) = delete;

	// ctor
	CPhysicalSplit(CMemoryPool *mp, CColRefArray *pdrgpcrDelete,
				   CColRefArray *pdrgpcrInsert, gpos::Ref<CColRef> pcrCtid,
				   gpos::Ref<CColRef> pcrSegmentId,
				   gpos::Ref<CColRef> pcrAction);

	// dtor
	~CPhysicalSplit() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalSplit;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalSplit";
	}

	// action column
	gpos::Ref<CColRef>
	PcrAction() const
	{
		return m_pcrAction;
	}

	// ctid column
	gpos::Ref<CColRef>
	PcrCtid() const
	{
		return m_pcrCtid;
	}

	// segmentid column
	gpos::Ref<CColRef>
	PcrSegmentId() const
	{
		return m_pcrSegmentId;
	}

	// deletion columns
	CColRefArray *
	PdrgpcrDelete() const
	{
		return m_pdrgpcrDelete;
	}

	// insertion columns
	CColRefArray *
	PdrgpcrInsert() const
	{
		return m_pdrgpcrInsert;
	}

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// hash function
	ULONG HashValue() const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required sort columns of the n-th child
	gpos::Ref<COrderSpec> PosRequired(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  gpos::Ref<COrderSpec> posRequired,
									  ULONG child_index,
									  CDrvdPropArray *pdrgpdpCtxt,
									  ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::Ref<COrderSpec> PosDerive(CMemoryPool *mp,
									CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		const gpos::Ref<CEnfdOrder> peo) const override;

	// compute required output columns of the n-th child
	gpos::Ref<CColRefSet> PcrsRequired(CMemoryPool *mp,
									   CExpressionHandle &exprhdl,
									   gpos::Ref<CColRefSet> pcrsRequired,
									   ULONG child_index,
									   CDrvdPropArray *pdrgpdpCtxt,
									   ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	gpos::Ref<CCTEReq> PcteRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
									gpos::Ref<CCTEReq> pcter, ULONG child_index,
									CDrvdPropArray *pdrgpdpCtxt,
									ULONG ulOptReq) const override;

	// compute required distribution of the n-th child
	gpos::Ref<CDistributionSpec> PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpec> pdsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::Ref<CRewindabilitySpec> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CRewindabilitySpec> prsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl,
						   gpos::Ref<CColRefSet> pcrsRequired,
						   ULONG ulOptReq) const override;


	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive distribution
	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::Ref<CRewindabilitySpec> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;


	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &,				 // exprhdl
		const gpos::Ref<CEnfdRewindability>	 // per
	) const override;

	// return true if operator passes through stats obtained from children,
	// this is used when computing stats during costing
	BOOL
	FPassThruStats() const override
	{
		return false;
	}

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CPhysicalSplit>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(COperator::EopPhysicalSplit == pop->Eopid());

		return dynamic_cast<gpos::Ref<CPhysicalSplit> >(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &os) const override;

};	// class CPhysicalSplit
}  // namespace gpopt

#endif	// !GPOS_CPhysicalSplit_H

// EOF
