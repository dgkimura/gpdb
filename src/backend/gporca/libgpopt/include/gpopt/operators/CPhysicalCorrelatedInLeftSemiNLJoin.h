//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalCorrelatedInLeftSemiNLJoin.h
//
//	@doc:
//		Physical Left Semi NLJ operator capturing correlated execution
//		with IN/ANY semantics
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalCorrelatedInLeftSemiNLJoin_H
#define GPOPT_CPhysicalCorrelatedInLeftSemiNLJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalLeftSemiNLJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalCorrelatedInLeftSemiNLJoin
//
//	@doc:
//		Physical left semi NLJ operator capturing correlated execution with
//		ANY/IN semantics
//
//---------------------------------------------------------------------------
class CPhysicalCorrelatedInLeftSemiNLJoin : public CPhysicalLeftSemiNLJoin
{
private:
	// columns from inner child used in correlated execution
	gpos::Ref<CColRefArray> m_pdrgpcrInner;

	// origin subquery id
	EOperatorId m_eopidOriginSubq;

public:
	CPhysicalCorrelatedInLeftSemiNLJoin(
		const CPhysicalCorrelatedInLeftSemiNLJoin &) = delete;

	// ctor
	CPhysicalCorrelatedInLeftSemiNLJoin(CMemoryPool *mp,
										gpos::Ref<CColRefArray> pdrgpcrInner,
										EOperatorId eopidOriginSubq)
		: CPhysicalLeftSemiNLJoin(mp),
		  m_pdrgpcrInner(std::move(pdrgpcrInner)),
		  m_eopidOriginSubq(eopidOriginSubq)
	{
		GPOS_ASSERT(nullptr != m_pdrgpcrInner);

		SetDistrRequests(UlDistrRequestsForCorrelatedJoin());
		GPOS_ASSERT(0 < UlDistrRequests());
	}

	// dtor
	~CPhysicalCorrelatedInLeftSemiNLJoin() override
	{
		;
	}

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalCorrelatedInLeftSemiNLJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalCorrelatedInLeftSemiNLJoin";
	}

	// match function
	BOOL
	Matches(COperator *pop) const override
	{
		if (pop->Eopid() == Eopid())
		{
			return m_pdrgpcrInner->Equals(
				gpos::dyn_cast<CPhysicalCorrelatedInLeftSemiNLJoin>(pop)
					->PdrgPcrInner());
		}

		return false;
	}

	// distribution matching type
	CEnfdDistribution::EDistributionMatching
	Edm(CReqdPropPlan *,   // prppInput
		ULONG,			   // child_index
		CDrvdPropArray *,  //pdrgpdpCtxt
		ULONG			   // ulOptReq
		) override
	{
		return CEnfdDistribution::EdmSatisfy;
	}

	gpos::Ref<CEnfdDistribution>
	Ped(CMemoryPool *mp, CExpressionHandle &exprhdl, CReqdPropPlan *prppInput,
		ULONG child_index, CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) override
	{
		return PedCorrelatedJoin(mp, exprhdl, prppInput, child_index,
								 pdrgpdpCtxt, ulOptReq);
	}

	// compute required distribution of the n-th child
	gpos::Ref<CDistributionSpec>
	PdsRequired(CMemoryPool *,		  // mp
				CExpressionHandle &,  // exprhdl,
				CDistributionSpec *,  // pdsRequired,
				ULONG,				  // child_index,
				CDrvdPropArray *,	  // pdrgpdpCtxt,
				ULONG				  //ulOptReq
	) const override
	{
		GPOS_RAISE(
			CException::ExmaInvalid, CException::ExmiInvalid,
			GPOS_WSZ_LIT(
				"PdsRequired should not be called for CPhysicalCorrelatedInLeftSemiNLJoin"));
		return nullptr;
	}

	// compute required rewindability of the n-th child
	gpos::Ref<CRewindabilitySpec>
	PrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
				CRewindabilitySpec *prsRequired, ULONG child_index,
				CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override
	{
		return PrsRequiredCorrelatedJoin(mp, exprhdl, prsRequired, child_index,
										 pdrgpdpCtxt, ulOptReq);
	}

	// conversion function
	static CPhysicalCorrelatedInLeftSemiNLJoin *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalCorrelatedInLeftSemiNLJoin == pop->Eopid());

		return dynamic_cast<CPhysicalCorrelatedInLeftSemiNLJoin *>(pop);
	}

	// return true if operator is a correlated NL Join
	BOOL
	FCorrelated() const override
	{
		return true;
	}

	// return required inner columns
	CColRefArray *
	PdrgPcrInner() const override
	{
		return m_pdrgpcrInner.get();
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
		(void) CUtils::OsPrintDrgPcr(os, m_pdrgpcrInner.get());
		os << ")";

		return os;
	}

};	// class CPhysicalCorrelatedInLeftSemiNLJoin

}  // namespace gpopt


#endif	// !GPOPT_CPhysicalCorrelatedInLeftSemiNLJoin_H

// EOF
