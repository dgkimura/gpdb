//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalCorrelatedLeftAntiSemiNLJoin.h
//
//	@doc:
//		Physical Left Anti Semi NLJ operator capturing correlated execution
//		of NOT EXISTS subqueries
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalCorrelatedLeftAntiSemiNLJoin_H
#define GPOPT_CPhysicalCorrelatedLeftAntiSemiNLJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalLeftAntiSemiNLJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalCorrelatedLeftAntiSemiNLJoin
//
//	@doc:
//		Physical left anti semi NLJ operator capturing correlated execution of
//		NOT EXISTS subqueries
//
//---------------------------------------------------------------------------
class CPhysicalCorrelatedLeftAntiSemiNLJoin : public CPhysicalLeftAntiSemiNLJoin
{
private:
	// columns from inner child used in correlated execution
	gpos::Ref<CColRefArray> m_pdrgpcrInner;

	// origin subquery id
	EOperatorId m_eopidOriginSubq;

public:
	CPhysicalCorrelatedLeftAntiSemiNLJoin(
		const CPhysicalCorrelatedLeftAntiSemiNLJoin &) = delete;

	// ctor
	CPhysicalCorrelatedLeftAntiSemiNLJoin(CMemoryPool *mp,
										  gpos::Ref<CColRefArray> pdrgpcrInner,
										  EOperatorId eopidOriginSubq)
		: CPhysicalLeftAntiSemiNLJoin(mp),
		  m_pdrgpcrInner(std::move(pdrgpcrInner)),
		  m_eopidOriginSubq(eopidOriginSubq)
	{
		GPOS_ASSERT(nullptr != m_pdrgpcrInner);

		SetDistrRequests(UlDistrRequestsForCorrelatedJoin());
		GPOS_ASSERT(0 < UlDistrRequests());
	}

	// dtor
	~CPhysicalCorrelatedLeftAntiSemiNLJoin() override
	{
		;
	}

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalCorrelatedLeftAntiSemiNLJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalCorrelatedLeftAntiSemiNLJoin";
	}

	// match function
	BOOL
	Matches(gpos::Ref<COperator> pop) const override
	{
		if (pop->Eopid() == Eopid())
		{
			return m_pdrgpcrInner->Equals(
				gpos::dyn_cast<CPhysicalCorrelatedLeftAntiSemiNLJoin>(pop)
					->PdrgPcrInner()
					.get());
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
				"PdsRequired should not be called for CPhysicalCorrelatedLeftAntiSemiNLJoin"));
		return nullptr;
	}

	// compute required rewindability of the n-th child
	gpos::Ref<CRewindabilitySpec>
	PrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
				CRewindabilitySpec *prsRequired, ULONG child_index,
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
		return m_pdrgpcrInner.get();
	}

	// print
	IOstream &
	OsPrint(IOstream &os) const override
	{
		os << this->SzId() << "(";
		(void) CUtils::OsPrintDrgPcr(os, m_pdrgpcrInner.get());
		os << ")";

		return os;
	}

	// origin subquery id
	EOperatorId
	EopidOriginSubq() const
	{
		return m_eopidOriginSubq;
	}

	// conversion function
	static gpos::Ref<CPhysicalCorrelatedLeftAntiSemiNLJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalCorrelatedLeftAntiSemiNLJoin == pop->Eopid());

		return gpos::dyn_cast<CPhysicalCorrelatedLeftAntiSemiNLJoin>(pop);
	}

};	// class CPhysicalCorrelatedLeftAntiSemiNLJoin

}  // namespace gpopt


#endif	// !GPOPT_CPhysicalCorrelatedLeftAntiSemiNLJoin_H

// EOF
