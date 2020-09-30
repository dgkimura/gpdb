//	Greenplum Database
//	Copyright (C) 2020 VMware Inc.

#include "gpopt/base/CDistributionSpecReplicated.h"

#include "gpopt/base/CDistributionSpecNonSingleton.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/operators/CPhysicalMotionBroadcast.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecReplicated::FSatisfies
//
//	@doc:
//		Check if this distribution satisfy the given one
//
//---------------------------------------------------------------------------
BOOL
CDistributionSpecReplicated::FSatisfies(const CDistributionSpec *pds) const
{
	GPOS_ASSERT(Ert() != EReplicatedType::ErtGeneral);

	if (Ert() == EReplicatedType::ErtTainted)
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
			case CDistributionSpec::EdtReplicated:
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
	else if (Ert() == EReplicatedType::ErtStrict)
	{
		if (Matches(pds))
		{
			// exact match implies satisfaction
			return true;
		 }

		if (EdtNonSingleton == pds->Edt())
		{
			// a replicated distribution satisfies the non-singleton
			// distribution, only if allowed by non-singleton distribution object
			return CDistributionSpecNonSingleton::PdsConvert(const_cast<CDistributionSpec *>(pds))->FAllowReplicated();
		}

		// replicated distribution satisfies a general replicated distribution spec
		if (EdtReplicated == pds->Edt())
		{
			return true;
		}

		// a replicated distribution satisfies any non-singleton one,
		// as well as singleton distributions that are not master-only
		return !(EdtSingleton == pds->Edt() &&
			   (dynamic_cast<const CDistributionSpecSingleton *>(pds))->FOnMaster());
	}

	return false;
}

void
CDistributionSpecReplicated::AppendEnforcers(CMemoryPool *mp,
											 CExpressionHandle &,  // exprhdl
											 CReqdPropPlan *
#ifdef GPOS_DEBUG
												 prpp
#endif	// GPOS_DEBUG
											 ,
											 CExpressionArray *pdrgpexpr,
											 CExpression *pexpr)
{
	GPOS_ASSERT(NULL != mp);
	GPOS_ASSERT(NULL != prpp);
	GPOS_ASSERT(NULL != pdrgpexpr);
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(!GPOS_FTRACE(EopttraceDisableMotions));
	GPOS_ASSERT(this == prpp->Ped()->PdsRequired() &&
				"required plan properties don't match enforced distribution spec");
	GPOS_ASSERT(Ert() != EReplicatedType::ErtTainted);

	if (GPOS_FTRACE(EopttraceDisableMotionBroadcast))
	{
		// broadcast Motion is disabled
		return;
	}

	pexpr->AddRef();
	CExpression *pexprMotion = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CPhysicalMotionBroadcast(mp), pexpr);
	pdrgpexpr->Append(pexprMotion);
}
// EOF
