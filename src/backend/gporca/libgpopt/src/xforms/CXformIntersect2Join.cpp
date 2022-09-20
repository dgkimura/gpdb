//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformIntersect2Join.cpp
//
//	@doc:
//		Implement the transformation of Intersect into a Join
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformIntersect2Join.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefComputed.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/exception.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalIntersect.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformIntersect2Join::CXformIntersect2Join
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformIntersect2Join::CXformIntersect2Join(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalIntersect(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // left relational child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp))  // right relational child
			  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformIntersect2Join::Transform
//
//	@doc:
//		Actual transformation that transforms an intersect into a join
//		over a group by over the inputs
//
//---------------------------------------------------------------------------
void
CXformIntersect2Join::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
								CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	GPOS_ASSERT(2 == pexpr->Arity());

	// extract components
	CExpression *pexprLeftChild = (*pexpr)[0];
	CExpression *pexprRightChild = (*pexpr)[1];

	CLogicalIntersect *popIntersect =
		gpos::dyn_cast<CLogicalIntersect>(pexpr->Pop());
	CColRef2dArray *pdrgpdrgpcrInput = popIntersect->PdrgpdrgpcrInput();

	// construct group by over the left and right expressions

	gpos::Ref<CExpression> pexprProjList =
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 GPOS_NEW(mp) CExpressionArray(mp));
	;
	;
	;
	;
	;

	gpos::Ref<CExpression> pexprLeftAgg = CUtils::PexprLogicalGbAggGlobal(
		mp, (*pdrgpdrgpcrInput)[0], pexprLeftChild, pexprProjList);
	gpos::Ref<CExpression> pexprRightAgg = CUtils::PexprLogicalGbAggGlobal(
		mp, (*pdrgpdrgpcrInput)[1], pexprRightChild, pexprProjList);

	gpos::Ref<CExpression> pexprScCond =
		CUtils::PexprConjINDFCond(mp, pdrgpdrgpcrInput);

	gpos::Ref<CExpression> pexprJoin = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), std::move(pexprLeftAgg),
		std::move(pexprRightAgg), std::move(pexprScCond));


	pxfres->Add(std::move(pexprJoin));
}

// EOF
