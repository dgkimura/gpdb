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
#include "gpos/common/owner.h"

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
	gpos::owner<CDistributionSpec *> m_pds;

	// order specs of child window functions
	gpos::owner<COrderSpecArray *> m_pdrgpos;

	// frames of child window functions
	gpos::owner<CWindowFrameArray *> m_pdrgpwf;

	// order spec to request from child
	gpos::owner<COrderSpec *> m_pos;

	// required columns in order/frame specs
	gpos::owner<CColRefSet *> m_pcrsRequiredLocal;

	// create local order spec
	void CreateOrderSpec(CMemoryPool *mp);

	// compute local required columns
	void ComputeRequiredLocalColumns(CMemoryPool *mp);

public:
	CPhysicalSequenceProject(const CPhysicalSequenceProject &) = delete;

	// ctor
	CPhysicalSequenceProject(CMemoryPool *mp,
							 gpos::owner<CDistributionSpec *> pds,
							 gpos::owner<COrderSpecArray *> pdrgpos,
							 gpos::owner<CWindowFrameArray *> pdrgpwf);

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
	gpos::pointer<CDistributionSpec *>
	Pds() const
	{
		return m_pds;
	}

	// order by keys
	gpos::pointer<COrderSpecArray *>
	Pdrgpos() const
	{
		return m_pdrgpos;
	}

	// frame specifications
	gpos::pointer<CWindowFrameArray *>
	Pdrgpwf() const
	{
		return m_pdrgpwf;
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

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
	gpos::owner<CColRefSet *> PcrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt, ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	gpos::owner<CCTEReq *> PcteRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CCTEReq *> pcter, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
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

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &,					   // exprhdl
		gpos::pointer<const CEnfdRewindability *>  // per
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
	static gpos::cast_func<CPhysicalSequenceProject *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalSequenceProject == pop->Eopid());

		return dynamic_cast<CPhysicalSequenceProject *>(pop);
	}

};	// class CPhysicalSequenceProject

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalSequenceProject_H

// EOF
