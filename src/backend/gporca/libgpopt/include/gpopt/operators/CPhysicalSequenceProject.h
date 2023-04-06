//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalSequenceProject.h
//
//	@doc:
//		Physical Sequence Project operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalSequenceProject_H
#define GPOPT_CPhysicalSequenceProject_H

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/CWindowFrame.h"
#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalSequenceProject
//
//	@doc:
//		Physical Sequence Project operator
//
//---------------------------------------------------------------------------
class CPhysicalSequenceProject : public CPhysical
{
private:
	// partition by keys
	gpos::Ref<CDistributionSpec> m_pds;

	// order specs of child window functions
	COrderSpecArray *m_pdrgpos;

	// frames of child window functions
	CWindowFrameArray *m_pdrgpwf;

	// order spec to request from child
	gpos::Ref<COrderSpec> m_pos;

	// required columns in order/frame specs
	gpos::Ref<CColRefSet> m_pcrsRequiredLocal;

	// create local order spec
	void CreateOrderSpec(CMemoryPool *mp);

	// compute local required columns
	void ComputeRequiredLocalColumns(CMemoryPool *mp);

public:
	CPhysicalSequenceProject(const CPhysicalSequenceProject &) = delete;

	// ctor
	CPhysicalSequenceProject(CMemoryPool *mp, gpos::Ref<CDistributionSpec> pds,
							 COrderSpecArray *pdrgpos,
							 CWindowFrameArray *pdrgpwf);

	// dtor
	~CPhysicalSequenceProject() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalSequenceProject;
	}

	// operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalSequenceProject";
	}

	// partition by keys
	gpos::Ref<CDistributionSpec>
	Pds() const
	{
		return m_pds;
	}

	// order by keys
	COrderSpecArray *
	Pdrgpos() const
	{
		return m_pdrgpos;
	}

	// frame specifications
	CWindowFrameArray *
	Pdrgpwf() const
	{
		return m_pdrgpwf;
	}

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// hashing function
	ULONG HashValue() const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

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

	// compute required sort order of the n-th child
	gpos::Ref<COrderSpec> PosRequired(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  gpos::Ref<COrderSpec> posRequired,
									  ULONG child_index,
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

	// derive sort order
	gpos::Ref<COrderSpec> PosDerive(CMemoryPool *mp,
									CExpressionHandle &exprhdl) const override;

	// derive distribution
	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::Ref<CRewindabilitySpec> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		const gpos::Ref<CEnfdOrder> peo) const override;

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

	// print
	IOstream &OsPrint(IOstream &os) const override;

	// conversion function
	static gpos::Ref<CPhysicalSequenceProject>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalSequenceProject == pop->Eopid());

		return dynamic_cast<gpos::Ref<CPhysicalSequenceProject> >(pop);
	}

};	// class CPhysicalSequenceProject

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalSequenceProject_H

// EOF
