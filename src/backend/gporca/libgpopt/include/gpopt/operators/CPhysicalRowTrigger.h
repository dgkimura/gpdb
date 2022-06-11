//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalRowTrigger.h
//
//	@doc:
//		Physical row-level trigger operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalRowTrigger_H
#define GPOPT_CPhysicalRowTrigger_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalRowTrigger
//
//	@doc:
//		Physical row-level trigger operator
//
//---------------------------------------------------------------------------
class CPhysicalRowTrigger : public CPhysical
{
private:
	// relation id on which triggers are to be executed
	gpos::owner<IMDId *> m_rel_mdid;

	// trigger type
	INT m_type;

	// old columns
	gpos::owner<CColRefArray *> m_pdrgpcrOld;

	// new columns
	gpos::owner<CColRefArray *> m_pdrgpcrNew;

	// required columns by local members
	gpos::owner<CColRefSet *> m_pcrsRequiredLocal;

public:
	CPhysicalRowTrigger(const CPhysicalRowTrigger &) = delete;

	// ctor
	CPhysicalRowTrigger(CMemoryPool *mp, IMDId *rel_mdid, INT type,
						CColRefArray *pdrgpcrOld, CColRefArray *pdrgpcrNew);

	// dtor
	~CPhysicalRowTrigger() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalRowTrigger;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalRowTrigger";
	}

	// relation id
	gpos::pointer<IMDId *>
	GetRelMdId() const
	{
		return m_rel_mdid;
	}

	// trigger type
	INT
	GetType() const
	{
		return m_type;
	}

	// old columns
	gpos::pointer<CColRefArray *>
	PdrgpcrOld() const
	{
		return m_pdrgpcrOld;
	}

	// new columns
	gpos::pointer<CColRefArray *>
	PdrgpcrNew() const
	{
		return m_pdrgpcrNew;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(COperator *pop) const override;

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
	gpos::owner<COrderSpec *> PosRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<COrderSpec *> posRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::owner<COrderSpec *> PosDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	// compute required output columns of the n-th child
	CColRefSet *PcrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
							 CColRefSet *pcrsRequired, ULONG child_index,
							 CDrvdPropArray *pdrgpdpCtxt,
							 ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	CCTEReq *PcteRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
						  CCTEReq *pcter, ULONG child_index,
						  CDrvdPropArray *pdrgpdpCtxt,
						  ULONG ulOptReq) const override;

	// compute required distribution of the n-th child
	gpos::owner<CDistributionSpec *> PdsRequired(CMemoryPool *mp,
												 CExpressionHandle &exprhdl,
												 CDistributionSpec *pdsRequired,
												 ULONG child_index,
												 CDrvdPropArray *pdrgpdpCtxt,
												 ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	CRewindabilitySpec *PrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
									CRewindabilitySpec *prsRequired,
									ULONG child_index,
									CDrvdPropArray *pdrgpdpCtxt,
									ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl, CColRefSet *pcrsRequired,
						   ULONG ulOptReq) const override;


	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive distribution
	CDistributionSpec *PdsDerive(CMemoryPool *mp,
								 CExpressionHandle &exprhdl) const override;

	// derive rewindability
	CRewindabilitySpec *PrsDerive(CMemoryPool *mp,
								  CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

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
		return true;
	}

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CPhysicalRowTrigger *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalRowTrigger == pop->Eopid());

		return dynamic_cast<CPhysicalRowTrigger *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CPhysicalRowTrigger
}  // namespace gpopt

#endif	// !GPOPT_CPhysicalRowTrigger_H

// EOF
