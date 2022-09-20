//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CDistributionSpecHashedNoOp.h"

#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPhysicalMotionHashDistribute.h"

using namespace gpopt;

CDistributionSpecHashedNoOp::CDistributionSpecHashedNoOp(
	gpos::Ref<CExpressionArray> pdrgpexpr)
	: CDistributionSpecHashed(std::move(pdrgpexpr), true)
{
}

CDistributionSpec::EDistributionType
CDistributionSpecHashedNoOp::Edt() const
{
	return CDistributionSpec::EdtHashedNoOp;
}

BOOL
CDistributionSpecHashedNoOp::Matches(const CDistributionSpec *pds) const
{
	return pds->Edt() == Edt();
}

void
CDistributionSpecHashedNoOp::AppendEnforcers(CMemoryPool *mp,
											 CExpressionHandle &exprhdl,
											 CReqdPropPlan *,
											 CExpressionArray *pdrgpexpr,
											 CExpression *pexpr)
{
	CDrvdProp *pdp = exprhdl.Pdp();
	CDistributionSpec *pdsChild = gpos::dyn_cast<CDrvdPropPlan>(pdp)->Pds();
	CDistributionSpecHashed *pdsChildHashed =
		dynamic_cast<CDistributionSpecHashed *>(pdsChild);

	if (nullptr == pdsChildHashed)
	{
		return;
	}

	gpos::Ref<CExpressionArray> pdrgpexprNoOpRedistributionColumns =
		pdsChildHashed->Pdrgpexpr();
	;
	gpos::Ref<CDistributionSpecHashedNoOp> pdsNoOp =
		GPOS_NEW(mp) CDistributionSpecHashedNoOp(
			std::move(pdrgpexprNoOpRedistributionColumns));
	;
	gpos::Ref<CExpression> pexprMotion = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CPhysicalMotionHashDistribute(mp, std::move(pdsNoOp)),
		pexpr);
	pdrgpexpr->Append(std::move(pexprMotion));
}
