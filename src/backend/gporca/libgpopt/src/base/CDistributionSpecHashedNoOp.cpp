//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CDistributionSpecHashedNoOp.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPhysicalMotionHashDistribute.h"

using namespace gpopt;

CDistributionSpecHashedNoOp::CDistributionSpecHashedNoOp(
	CExpressionArray *pdrgpexpr)
	: CDistributionSpecHashed(pdrgpexpr, true)
{
}

CDistributionSpec::EDistributionType
CDistributionSpecHashedNoOp::Edt() const
{
	return CDistributionSpec::EdtHashedNoOp;
}

BOOL
CDistributionSpecHashedNoOp::Matches(
	const gpos::Ref<CDistributionSpec> pds) const
{
	return pds->Edt() == Edt();
}

void
CDistributionSpecHashedNoOp::AppendEnforcers(CMemoryPool *mp,
											 CExpressionHandle &exprhdl,
											 gpos::Ref<CReqdPropPlan>,
											 CExpressionArray *pdrgpexpr,
											 gpos::Ref<CExpression> pexpr)
{
	gpos::Ref<CDrvdProp> pdp = exprhdl.Pdp();
	gpos::Ref<CDistributionSpec> pdsChild = CDrvdPropPlan::Pdpplan(pdp)->Pds();
	gpos::Ref<CDistributionSpecHashed> pdsChildHashed =
		dynamic_cast<gpos::Ref<CDistributionSpecHashed> >(pdsChild);

	if (nullptr == pdsChildHashed)
	{
		return;
	}

	CExpressionArray *pdrgpexprNoOpRedistributionColumns =
		pdsChildHashed->Pdrgpexpr();
	pdrgpexprNoOpRedistributionColumns->AddRef();
	gpos::Ref<CDistributionSpecHashedNoOp> pdsNoOp = GPOS_NEW(mp)
		CDistributionSpecHashedNoOp(pdrgpexprNoOpRedistributionColumns);
	pexpr->AddRef();
	gpos::Ref<CExpression> pexprMotion = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CPhysicalMotionHashDistribute(mp, pdsNoOp), pexpr);
	pdrgpexpr->Append(pexprMotion);
}
