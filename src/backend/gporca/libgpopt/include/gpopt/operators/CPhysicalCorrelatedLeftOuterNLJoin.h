//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalCorrelatedLeftOuterNLJoin.h
//
//	@doc:
//		Physical Left Outer NLJ  operator capturing correlated execution
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalCorrelatedLeftOuterNLJoin_H
#define GPOPT_CPhysicalCorrelatedLeftOuterNLJoin_H

#include "gpos/base.h"

#include "gpopt/operators/CPhysicalLeftOuterNLJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalCorrelatedLeftOuterNLJoin
//
//	@doc:
//		Physical left outer NLJ operator capturing correlated execution
//
//---------------------------------------------------------------------------
class CPhysicalCorrelatedLeftOuterNLJoin : public CPhysicalLeftOuterNLJoin
{
private:
	// columns from inner child used in correlated execution
	gpos::Ref<CColRefArray> m_pdrgpcrInner;

	// origin subquery id
	EOperatorId m_eopidOriginSubq;

public:
	CPhysicalCorrelatedLeftOuterNLJoin(
		const CPhysicalCorrelatedLeftOuterNLJoin &) = delete;

	// ctor
	CPhysicalCorrelatedLeftOuterNLJoin(CMemoryPool *mp,
									   gpos::Ref<CColRefArray> pdrgpcrInner,
									   EOperatorId eopidOriginSubq)
		: CPhysicalLeftOuterNLJoin(mp),
		  m_pdrgpcrInner(pdrgpcrInner),
		  m_eopidOriginSubq(eopidOriginSubq)
	{
		GPOS_ASSERT(nullptr != pdrgpcrInner);

		SetDistrRequests(UlDistrRequestsForCorrelatedJoin());
		GPOS_ASSERT(0 < UlDistrRequests());
	}

	// dtor
	~CPhysicalCorrelatedLeftOuterNLJoin() override
	{
	}

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalCorrelatedLeftOuterNLJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalCorrelatedLeftOuterNLJoin";
	}

	// match function
	BOOL
	Matches(gpos::Ref<COperator> pop) const override
	{
		if (pop->Eopid() == Eopid())
		{
			return m_pdrgpcrInner->Equals(
				CPhysicalCorrelatedLeftOuterNLJoin::PopConvert(pop)
					->PdrgPcrInner());
		}

		return false;
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
				"PdsRequired should not be called for CPhysicalCorrelatedLeftOuterNLJoin"));
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

	// conversion function
	static gpos::Ref<CPhysicalCorrelatedLeftOuterNLJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalCorrelatedLeftOuterNLJoin == pop->Eopid());

		return gpos::dyn_cast<CPhysicalCorrelatedLeftOuterNLJoin>(pop);
	}

};	// class CPhysicalCorrelatedLeftOuterNLJoin

}  // namespace gpopt


#endif	// !GPOPT_CPhysicalCorrelatedLeftOuterNLJoin_H

// EOF
