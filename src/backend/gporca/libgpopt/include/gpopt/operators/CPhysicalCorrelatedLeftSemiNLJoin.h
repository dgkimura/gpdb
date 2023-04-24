//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalCorrelatedLeftSemiNLJoin.h
//
//	@doc:
//		Physical Left Semi NLJ operator capturing correlated execution
//		for EXISTS subqueries
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalCorrelatedLeftSemiNLJoin_H
#define GPOPT_CPhysicalCorrelatedLeftSemiNLJoin_H

#include "gpos/base.h"

#include "gpopt/operators/CPhysicalLeftSemiNLJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalCorrelatedLeftSemiNLJoin
//
//	@doc:
//		Physical left semi NLJ operator capturing correlated execution for
//		EXISTS subqueries
//
//---------------------------------------------------------------------------
class CPhysicalCorrelatedLeftSemiNLJoin : public CPhysicalLeftSemiNLJoin
{
private:
	// columns from inner child used in correlated execution
	gpos::Ref<CColRefArray> m_pdrgpcrInner;

	// origin subquery id
	EOperatorId m_eopidOriginSubq;

public:
	CPhysicalCorrelatedLeftSemiNLJoin(
		const CPhysicalCorrelatedLeftSemiNLJoin &) = delete;

	// ctor
	CPhysicalCorrelatedLeftSemiNLJoin(CMemoryPool *mp,
									  gpos::Ref<CColRefArray> pdrgpcrInner,
									  EOperatorId eopidOriginSubq)
		: CPhysicalLeftSemiNLJoin(mp),
		  m_pdrgpcrInner(pdrgpcrInner),
		  m_eopidOriginSubq(eopidOriginSubq)
	{
		GPOS_ASSERT(nullptr != pdrgpcrInner);

		SetDistrRequests(UlDistrRequestsForCorrelatedJoin());
		GPOS_ASSERT(0 < UlDistrRequests());
	}

	// dtor
	~CPhysicalCorrelatedLeftSemiNLJoin() override
	{
	}

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalCorrelatedLeftSemiNLJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalCorrelatedLeftSemiNLJoin";
	}

	// match function
	BOOL
	Matches(gpos::Ref<COperator> pop) const override
	{
		if (pop->Eopid() == Eopid())
		{
			return m_pdrgpcrInner->Equals(
				CPhysicalCorrelatedLeftSemiNLJoin::PopConvert(pop)
					->PdrgPcrInner());
		}

		return false;
	}

	// distribution matching type
	CEnfdDistribution::EDistributionMatching Edm(
		gpos::Ref<CReqdPropPlan>,	// prppInput
		ULONG,						// child_index
		gpos::Ref<CDrvdPropArray>,	//pdrgpdpCtxt
		ULONG						// ulOptReq
		) override
	{
		return CEnfdDistribution::EdmSatisfy;
	}

	gpos::Ref<CEnfdDistribution>
	Ped(CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CReqdPropPlan> prppInput, ULONG child_index,
		gpos::Ref<CDrvdPropArray> pdrgpdpCtxt, ULONG ulOptReq) override
	{
		return PedCorrelatedJoin(mp, exprhdl, prppInput, child_index,
								 pdrgpdpCtxt, ulOptReq);
	}

	// compute required distribution of the n-th child
	gpos::Ref<CDistributionSpec>
	PdsRequired(CMemoryPool *,				   // mp
				CExpressionHandle &,		   // exprhdl,
				gpos::Ref<CDistributionSpec>,  // pdsRequired,
				ULONG,						   // child_index,
				gpos::Ref<CDrvdPropArray>,	   // pdrgpdpCtxt,
				ULONG						   //ulOptReq
	) const override
	{
		GPOS_RAISE(
			CException::ExmaInvalid, CException::ExmiInvalid,
			GPOS_WSZ_LIT(
				"PdsRequired should not be called for CPhysicalCorrelatedLeftSemiNLJoin"));
		return nullptr;
	}

	// compute required rewindability of the n-th child
	gpos::Ref<CRewindabilitySpec>
	PrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
				gpos::Ref<CRewindabilitySpec> prsRequired, ULONG child_index,
				gpos::Ref<CDrvdPropArray> pdrgpdpCtxt,
				ULONG ulOptReq) const override
	{
		return PrsRequiredCorrelatedJoin(mp, exprhdl, prsRequired, child_index,
										 pdrgpdpCtxt, ulOptReq);
	}

	// return true if operator is a correlated NL Join
	BOOL
	FCorrelated() const override
	{
		return true;
	}

	// return required inner columns
	gpos::Ref<CColRefArray>
	PdrgPcrInner() const override
	{
		return m_pdrgpcrInner;
	}

	// origin subquery id
	EOperatorId
	EopidOriginSubq() const
	{
		return m_eopidOriginSubq;
	}

	// print
	IOstream &
	OsPrint(IOstream &os) const override
	{
		os << this->SzId() << "(";
		(void) CUtils::OsPrintDrgPcr(os, m_pdrgpcrInner);
		os << ")";

		return os;
	}

	// conversion function
	static gpos::Ref<CPhysicalCorrelatedLeftSemiNLJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalCorrelatedLeftSemiNLJoin == pop->Eopid());

		return gpos::dyn_cast<CPhysicalCorrelatedLeftSemiNLJoin>(pop);
	}

};	// class CPhysicalCorrelatedLeftSemiNLJoin

}  // namespace gpopt


#endif	// !GPOPT_CPhysicalCorrelatedLeftSemiNLJoin_H

// EOF
