//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalCTEProducer.h
//
//	@doc:
//		Physical CTE producer operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalCTEProducer_H
#define GPOPT_CPhysicalCTEProducer_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalCTEProducer
//
//	@doc:
//		CTE producer operator
//
//---------------------------------------------------------------------------
class CPhysicalCTEProducer : public CPhysical
{
private:
	// cte identifier
	ULONG m_id;

	// cte columns
	gpos::owner<CColRefArray *> m_pdrgpcr;

	// set representation of cte columns
	gpos::owner<CColRefSet *> m_pcrs;

public:
	CPhysicalCTEProducer(const CPhysicalCTEProducer &) = delete;

	// ctor
	CPhysicalCTEProducer(CMemoryPool *mp, ULONG id,
						 gpos::owner<CColRefArray *> colref_array);

	// dtor
	~CPhysicalCTEProducer() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalCTEProducer;
	}

	const CHAR *
	SzId() const override
	{
		return "CPhysicalCTEProducer";
	}

	// cte identifier
	ULONG
	UlCTEId() const
	{
		return m_id;
	}

	// cte columns
	gpos::pointer<CColRefArray *>
	Pdrgpcr() const
	{
		return m_pdrgpcr;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
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

	// derive cte map
	gpos::owner<CCTEMap *> PcmDerive(CMemoryPool *mp,
									 CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdRewindability *> per) const override;

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
	static gpos::cast_func<CPhysicalCTEProducer *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalCTEProducer == pop->Eopid());

		return dynamic_cast<CPhysicalCTEProducer *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CPhysicalCTEProducer

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalCTEProducer_H

// EOF
