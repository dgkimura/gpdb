//	Greenplum Database
//	Copyright (C) 2020 VMware Inc.

#include "gpopt/base/CDistributionSpecTaintedReplicated.h"

#include "gpopt/base/CDistributionSpecNonSingleton.h"
#include "gpopt/base/CDistributionSpecSingleton.h"

namespace gpopt
{

BOOL
CDistributionSpecTaintedReplicated::FSatisfies(const CDistributionSpec *pds) const
{
	// TaintedReplicated::FSatisfies logic is similar to Replicated::FSatisifes
	// except that Replicated can match and satisfy another Replicated Spec.
	// However, Tainted will never satisfy another TaintedReplicated or
	// Replicated.
	switch (pds->Edt())
	{
		default:
			return false;
		case CDistributionSpec::EdtAny:
			// tainted replicated distribution satisfies an any required distribution spec
			return true;
		case CDistributionSpec::EdtGeneralReplicated:
			// tainted replicated distribution satisfies a general replicated distribution spec
			return true;
		case CDistributionSpec::EdtNonSingleton:
			// a tainted replicated distribution satisfies the non-singleton
			// distribution, only if allowed by non-singleton distribution object
			return CDistributionSpecNonSingleton::PdsConvert(pds)->FAllowReplicated();
		case CDistributionSpec::EdtSingleton:
			// a tainted replicated distribution satisfies singleton
			// distributions that are not master-only
			return CDistributionSpecSingleton::PdssConvert(pds)->Est() == CDistributionSpecSingleton::EstSegment;
	}
}

void
CDistributionSpecTaintedReplicated::AppendEnforcers(gpos::CMemoryPool *,
													CExpressionHandle &,
													CReqdPropPlan *,
													CExpressionArray *,
													CExpression *)
{
	GPOS_ASSERT(!"CDistributionSpecTaintedReplicated is derive-only, cannot be required");
}

} // namespace gpopt
