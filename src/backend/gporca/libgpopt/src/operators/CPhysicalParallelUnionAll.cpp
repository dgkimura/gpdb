//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/operators/CPhysicalParallelUnionAll.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecHashedNoOp.h"
#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/CDistributionSpecStrictHashed.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CStrictHashedDistributions.h"

namespace gpopt
{
CPhysicalParallelUnionAll::CPhysicalParallelUnionAll(
	CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput)
	: CPhysicalUnionAll(mp, pdrgpcrOutput, pdrgpdrgpcrInput),
	  m_pdrgpds(GPOS_NEW(mp) CStrictHashedDistributions(mp, pdrgpcrOutput.get(),
														pdrgpdrgpcrInput.get()))
{
	// ParallelUnionAll creates two distribution requests to enforce distribution of its children:
	// (1) (StrictHashed, StrictHashed, ...): used to force redistribute motions that mirror the
	//     output columns
	// (2) (HashedNoOp, HashedNoOp, ...): used to force redistribution motions that mirror the
	//     underlying distribution of each relational child

	SetDistrRequests(2);
}

COperator::EOperatorId
CPhysicalParallelUnionAll::Eopid() const
{
	return EopPhysicalParallelUnionAll;
}

const CHAR *
CPhysicalParallelUnionAll::SzId() const
{
	return "CPhysicalParallelUnionAll";
}

gpos::Ref<CDistributionSpec>
CPhysicalParallelUnionAll::PdsRequired(CMemoryPool *mp, CExpressionHandle &,
									   CDistributionSpec *, ULONG child_index,
									   CDrvdPropArray *, ULONG ulOptReq) const
{
	if (0 == ulOptReq)
	{
		gpos::Ref<CDistributionSpec> pdsChild = (*m_pdrgpds)[child_index];
		;
		return pdsChild;
	}
	else
	{
		CColRefArray *pdrgpcrChildInputColumns =
			(*PdrgpdrgpcrInput())[child_index].get();
		gpos::Ref<CExpressionArray> pdrgpexprFakeRequestedColumns =
			GPOS_NEW(mp) CExpressionArray(mp);

		CColRef *pcrFirstColumn = (*pdrgpcrChildInputColumns)[0];
		gpos::Ref<CExpression> pexprScalarIdent =
			CUtils::PexprScalarIdent(mp, pcrFirstColumn);
		pdrgpexprFakeRequestedColumns->Append(std::move(pexprScalarIdent));

		return GPOS_NEW(mp) CDistributionSpecHashedNoOp(
			std::move(pdrgpexprFakeRequestedColumns));
	}
}

CEnfdDistribution::EDistributionMatching
CPhysicalParallelUnionAll::Edm(CReqdPropPlan *,	  // prppInput
							   ULONG,			  // child_index
							   CDrvdPropArray *,  //pdrgpdpCtxt
							   ULONG			  // ulOptReq
)
{
	return CEnfdDistribution::EdmExact;
}

CPhysicalParallelUnionAll::~CPhysicalParallelUnionAll()
{
	;
}
}  // namespace gpopt
