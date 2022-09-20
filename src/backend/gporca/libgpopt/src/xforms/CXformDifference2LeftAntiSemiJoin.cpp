//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformDifference2LeftAntiSemiJoin.cpp
//
//	@doc:
//		Implementation of the transformation that takes a logical difference and
//		converts it into an aggregate over a left anti-semi join
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformDifference2LeftAntiSemiJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/exception.h"
#include "gpopt/operators/CLogicalDifference.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDifference2LeftAntiSemiJoin::CXformDifference2LeftAntiSemiJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformDifference2LeftAntiSemiJoin::CXformDifference2LeftAntiSemiJoin(
	CMemoryPool *mp)
	:  // pattern
	  CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalDifference(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDifference2LeftAntiSemiJoin::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformDifference2LeftAntiSemiJoin::Transform(
	gpos::pointer<CXformContext *> pxfctxt,
	gpos::pointer<CXformResult *> pxfres,
	gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// TODO: Oct 24th 2012, we currently only handle difference all
	//  operators with two children
	GPOS_ASSERT(2 == pexpr->Arity());

	// extract components
	CExpression *pexprLeftChild = (*pexpr)[0];
	CExpression *pexprRightChild = (*pexpr)[1];

	gpos::pointer<CLogicalDifference *> popDifference =
		gpos::dyn_cast<CLogicalDifference>(pexpr->Pop());
	CColRefArray *pdrgpcrOutput = popDifference->PdrgpcrOutput();
	gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput =
		popDifference->PdrgpdrgpcrInput();

	// generate the scalar condition for the left anti-semi join
	gpos::owner<CExpression *> pexprScCond =
		CUtils::PexprConjINDFCond(mp, pdrgpdrgpcrInput);

	pexprLeftChild->AddRef();
	pexprRightChild->AddRef();

	// assemble the new left anti-semi join logical operator
	gpos::owner<CExpression *> pexprLASJ = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalLeftAntiSemiJoin(mp),
					pexprLeftChild, pexprRightChild, std::move(pexprScCond));

	// assemble the aggregate operator
	pdrgpcrOutput->AddRef();

	gpos::owner<CExpression *> pexprProjList =
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 GPOS_NEW(mp) CExpressionArray(mp));

	gpos::owner<CExpression *> pexprAgg = CUtils::PexprLogicalGbAggGlobal(
		mp, pdrgpcrOutput, std::move(pexprLASJ), std::move(pexprProjList));

	// add alternative to results
	pxfres->Add(std::move(pexprAgg));
}

// EOF
