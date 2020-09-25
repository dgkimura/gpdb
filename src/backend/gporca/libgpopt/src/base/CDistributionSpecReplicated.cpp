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
CDistributionSpecReplicated::FSatisfies(const CDistributionSpec *pdss) const
{
	GPOS_ASSERT(!"General Replicated distribution cannot be derived");

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
