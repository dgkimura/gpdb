//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CDistributionSpecHashedNoOp.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdPropCtxtPlan.h"
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
CDistributionSpecHashedNoOp::Matches(
	const gpos::Ref<CDistributionSpec> pds) const
{
	return pds->Edt() == Edt();
}

void
CDistributionSpecHashedNoOp::AppendEnforcers(
	CMemoryPool *mp, CExpressionHandle &exprhdl, gpos::Ref<CReqdPropPlan>,
	gpos::Ref<CExpressionArray> pdrgpexpr, gpos::Ref<CExpression> pexpr)
{
	gpos::Ref<CDrvdProp> pdp = exprhdl.Pdp();
	gpos::Ref<CDistributionSpec> pdsChild =
		gpos::dyn_cast<CDrvdPropPlan>(pdp)->Pds();
	gpos::Ref<CDistributionSpecHashed> pdsChildHashed =
		gpos::dyn_cast<CDistributionSpecHashed>(pdsChild);

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
