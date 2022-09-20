//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalInnerIndexNLJoin.h
//
//	@doc:
//		Inner index nested-loops join operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalInnerIndexNLJoin_H
#define GPOPT_CPhysicalInnerIndexNLJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalInnerNLJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalInnerIndexNLJoin
//
//	@doc:
//		Inner index nested-loops join operator
//
//---------------------------------------------------------------------------
class CPhysicalInnerIndexNLJoin : public CPhysicalInnerNLJoin
{
private:
	// columns from outer child used for index lookup in inner child
	gpos::Ref<CColRefArray> m_pdrgpcrOuterRefs;

	// a copy of the original join predicate that has been pushed down to the inner side
	gpos::Ref<CExpression> m_origJoinPred;

public:
	CPhysicalInnerIndexNLJoin(const CPhysicalInnerIndexNLJoin &) = delete;

	// ctor
	CPhysicalInnerIndexNLJoin(CMemoryPool *mp,
							  gpos::Ref<CColRefArray> colref_array,
							  gpos::Ref<CExpression> origJoinPred);

	// dtor
	~CPhysicalInnerIndexNLJoin() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalInnerIndexNLJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalInnerIndexNLJoin";
	}

	// match function
	BOOL Matches(COperator *pop) const override;

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
	static CPhysicalInnerIndexNLJoin *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(EopPhysicalInnerIndexNLJoin == pop->Eopid());

		return dynamic_cast<CPhysicalInnerIndexNLJoin *>(pop);
	}

	CExpression *
	OrigJoinPred()
	{
		return m_origJoinPred.get();
	}

};	// class CPhysicalInnerIndexNLJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalInnerIndexNLJoin_H

// EOF
