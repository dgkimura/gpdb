//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformExpandNAryJoinGreedy.cpp
//
//	@doc:
//		Implementation of n-ary join expansion based on cardinality
//		of intermediate results and delay cross joins to
//		the end
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformExpandNAryJoinGreedy.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionUtils.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/CPatternMultiTree.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/xforms/CJoinOrderGreedy.h"
#include "gpopt/xforms/CXformUtils.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandNAryJoinGreedy::CXformExpandNAryJoinGreedy
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformExpandNAryJoinGreedy::CXformExpandNAryJoinGreedy(CMemoryPool *pmp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(pmp) CExpression(
			  pmp, GPOS_NEW(pmp) CLogicalNAryJoin(pmp),
			  GPOS_NEW(pmp)
				  CExpression(pmp, GPOS_NEW(pmp) CPatternMultiTree(pmp)),
			  GPOS_NEW(pmp) CExpression(pmp, GPOS_NEW(pmp) CPatternTree(pmp))))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandNAryJoinGreedy::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformExpandNAryJoinGreedy::Exfp(CExpressionHandle &exprhdl) const
{
	return CXformUtils::ExfpExpandJoinOrder(exprhdl, this);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandNAryJoinGreedy::Transform
//
//	@doc:
//		Actual transformation of n-ary join to cluster of inner joins
//
//---------------------------------------------------------------------------
void
CXformExpandNAryJoinGreedy::Transform(gpos::Ref<CXformContext> pxfctxt,
									  gpos::Ref<CXformResult> pxfres,
									  gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *pmp = pxfctxt->Pmp();

	const ULONG ulArity = pexpr->Arity();
	GPOS_ASSERT(ulArity >= 3);

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(pmp) CExpressionArray(pmp);
	for (ULONG ul = 0; ul < ulArity - 1; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		;
		pdrgpexpr->Append(pexprChild);
	}

	gpos::Ref<CExpression> pexprScalar = (*pexpr)[ulArity - 1];
	gpos::Ref<CExpressionArray> pdrgpexprPreds =
		CPredicateUtils::PdrgpexprConjuncts(pmp, pexprScalar);

	// create a join order based on cardinality of intermediate results
	CJoinOrderGreedy jomc(pmp, pdrgpexpr, pdrgpexprPreds);
	gpos::Ref<CExpression> pexprResult = jomc.PexprExpand();

	if (nullptr != pexprResult)
	{
		// normalize resulting expression
		gpos::Ref<CExpression> pexprNormalized =
			CNormalizer::PexprNormalize(pmp, pexprResult.get());
		;
		pxfres->Add(std::move(pexprNormalized));
	}
}

// EOF
