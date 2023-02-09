//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 VMware, Inc. or its affiliates.
//
//	Left outer index nested-loops join operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalLeftOuterIndexNLJoin_H
#define GPOPT_CPhysicalLeftOuterIndexNLJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalLeftOuterNLJoin.h"

namespace gpopt
{
class CPhysicalLeftOuterIndexNLJoin : public CPhysicalLeftOuterNLJoin
{
private:
	// columns from outer child used for index lookup in inner child
	gpos::Ref<CColRefArray> m_pdrgpcrOuterRefs;

	// a copy of the original join predicate that has been pushed down to the inner side
	gpos::Ref<CExpression> m_origJoinPred;

public:
	CPhysicalLeftOuterIndexNLJoin(const CPhysicalLeftOuterIndexNLJoin &) =
		delete;

	// ctor
	CPhysicalLeftOuterIndexNLJoin(CMemoryPool *mp,
								  gpos::Ref<CColRefArray> colref_array,
								  gpos::Ref<CExpression> origJoinPred);

	// dtor
	~CPhysicalLeftOuterIndexNLJoin() override;

	CEnfdProp::EPropEnforcingType EpetDistribution(
		CExpressionHandle &exprhdl,
		const CEnfdDistribution *ped) const override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalLeftOuterIndexNLJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalLeftOuterIndexNLJoin";
	}

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// outer column references accessor
	CColRefArray *
	PdrgPcrOuterRefs() const
	{
		return m_pdrgpcrOuterRefs.get();
	}

	// compute required distribution of the n-th child
	gpos::Ref<CDistributionSpec> PdsRequired(CMemoryPool *mp,
											 CExpressionHandle &exprhdl,
											 CDistributionSpec *pdsRequired,
											 ULONG child_index,
											 CDrvdPropArray *pdrgpdpCtxt,
											 ULONG ulOptReq) const override;

	gpos::Ref<CEnfdDistribution> Ped(CMemoryPool *mp,
									 CExpressionHandle &exprhdl,
									 CReqdPropPlan *prppInput,
									 ULONG child_index,
									 CDrvdPropArray *pdrgpdpCtxt,
									 ULONG ulDistrReq) override;

	// execution order of children
	EChildExecOrder
	Eceo() const override
	{
		// we optimize inner (right) child first to be able to match child hashed distributions
		return EceoRightToLeft;
	}

	// conversion function
	static CPhysicalLeftOuterIndexNLJoin *
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(EopPhysicalLeftOuterIndexNLJoin == pop->Eopid());

		return dynamic_cast<CPhysicalLeftOuterIndexNLJoin *>(pop);
	}

	gpos::Ref<CExpression>
	OrigJoinPred()
	{
		return m_origJoinPred.get();
	}

};	// class CPhysicalLeftOuterIndexNLJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalLeftOuterIndexNLJoin_H

// EOF
