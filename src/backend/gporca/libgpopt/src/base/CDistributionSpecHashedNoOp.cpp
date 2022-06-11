//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CDistributionSpecHashedNoOp.h"

#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPhysicalMotionHashDistribute.h"

using namespace gpopt;

CDistributionSpecHashedNoOp::CDistributionSpecHashedNoOp(
	gpos::owner<CExpressionArray *> pdrgpexpr)
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
	gpos::pointer<const CDistributionSpec *> pds) const
{
	return pds->Edt() == Edt();
}

void
CDistributionSpecHashedNoOp::AppendEnforcers(
	CMemoryPool *mp, CExpressionHandle &exprhdl, gpos::pointer<CReqdPropPlan *>,
	gpos::pointer<CExpressionArray *> pdrgpexpr,
	gpos::pointer<CExpression *> pexpr)
{
	gpos::pointer<CDrvdProp *> pdp = exprhdl.Pdp();
	gpos::pointer<CDistributionSpec *> pdsChild =
		gpos::dyn_cast<CDrvdPropPlan>(pdp)->Pds();
	gpos::pointer<CDistributionSpecHashed *> pdsChildHashed =
		dynamic_cast<CDistributionSpecHashed *>(pdsChild);

	if (nullptr == pdsChildHashed)
	{
		return;
	}

	gpos::owner<CExpressionArray *> pdrgpexprNoOpRedistributionColumns =
		pdsChildHashed->Pdrgpexpr();
	pdrgpexprNoOpRedistributionColumns->AddRef();
	gpos::owner<CDistributionSpecHashedNoOp *> pdsNoOp =
		GPOS_NEW(mp) CDistributionSpecHashedNoOp(
			std::move(pdrgpexprNoOpRedistributionColumns));
	pexpr->AddRef();
	gpos::owner<CExpression *> pexprMotion = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CPhysicalMotionHashDistribute(mp, std::move(pdsNoOp)),
		pexpr);
	pdrgpexpr->Append(std::move(pexprMotion));
}
