//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.


#ifndef GPOPT_CDistributionSpecHashedNoOp_H
#define GPOPT_CDistributionSpecHashedNoOp_H

#include "gpopt/base/CDistributionSpecHashed.h"

namespace gpopt
{
class CDistributionSpecHashedNoOp : public CDistributionSpecHashed
{
public:
	CDistributionSpecHashedNoOp(CExpressionArray *pdrgpexr);

	EDistributionType Edt() const override;

	BOOL Matches(const gpos::Ref<CDistributionSpec> pds) const override;

	const CHAR *
	SzId() const override
	{
		return "HASHED NO-OP";
	}

	void AppendEnforcers(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 gpos::Ref<CReqdPropPlan> prpp,
						 CExpressionArray *pdrgpexpr,
						 gpos::Ref<CExpression> pexpr) override;
};
}  // namespace gpopt

#endif
