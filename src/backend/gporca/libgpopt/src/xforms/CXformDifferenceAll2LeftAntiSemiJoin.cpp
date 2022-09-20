//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformDifferenceAll2LeftAntiSemiJoin.cpp
//
//	@doc:
//		Implementation of the transformation a logical difference all into LASJ
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformDifferenceAll2LeftAntiSemiJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/exception.h"
#include "gpopt/operators/CLogicalDifferenceAll.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/xforms/CXformIntersectAll2LeftSemiJoin.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDifferenceAll2LeftAntiSemiJoin::CXformDifferenceAll2LeftAntiSemiJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformDifferenceAll2LeftAntiSemiJoin::CXformDifferenceAll2LeftAntiSemiJoin(
	CMemoryPool *mp)
	:  // pattern
	  CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalDifferenceAll(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDifferenceAll2LeftAntiSemiJoin::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformDifferenceAll2LeftAntiSemiJoin::Transform(
	gpos::pointer<CXformContext *> pxfctxt,
	gpos::pointer<CXformResult *> pxfres,
	gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// TODO: , Jan 8th 2013, we currently only handle difference all
	//  operators with two children
	GPOS_ASSERT(2 == pexpr->Arity());

	// extract components
	CExpression *pexprLeftChild = (*pexpr)[0];
	CExpression *pexprRightChild = (*pexpr)[1];

	gpos::pointer<CLogicalDifferenceAll *> popDifferenceAll =
		gpos::dyn_cast<CLogicalDifferenceAll>(pexpr->Pop());
	gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput =
		popDifferenceAll->PdrgpdrgpcrInput();

	gpos::owner<CExpression *> pexprLeftWindow =
		CXformUtils::PexprWindowWithRowNumber(mp, pexprLeftChild,
											  (*pdrgpdrgpcrInput)[0]);
	gpos::owner<CExpression *> pexprRightWindow =
		CXformUtils::PexprWindowWithRowNumber(mp, pexprRightChild,
											  (*pdrgpdrgpcrInput)[1]);

	gpos::owner<CColRef2dArray *> pdrgpdrgpcrInputNew =
		GPOS_NEW(mp) CColRef2dArray(mp);
	gpos::owner<CColRefArray *> pdrgpcrLeftNew =
		CUtils::PdrgpcrExactCopy(mp, (*pdrgpdrgpcrInput)[0]);
	pdrgpcrLeftNew->Append(CXformUtils::PcrProjectElement(
		pexprLeftWindow, 0 /* row_number window function*/));

	gpos::owner<CColRefArray *> pdrgpcrRightNew =
		CUtils::PdrgpcrExactCopy(mp, (*pdrgpdrgpcrInput)[1]);
	pdrgpcrRightNew->Append(CXformUtils::PcrProjectElement(
		pexprRightWindow, 0 /* row_number window function*/));

	pdrgpdrgpcrInputNew->Append(std::move(pdrgpcrLeftNew));
	pdrgpdrgpcrInputNew->Append(std::move(pdrgpcrRightNew));

	// generate the scalar condition for the left anti-semi join
	gpos::owner<CExpression *> pexprScCond =
		CUtils::PexprConjINDFCond(mp, pdrgpdrgpcrInputNew);

	// assemble the new left anti-semi join logical operator
	gpos::owner<CExpression *> pexprLASJ = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalLeftAntiSemiJoin(mp),
					std::move(pexprLeftWindow), std::move(pexprRightWindow),
					std::move(pexprScCond));

	// clean up
	pdrgpdrgpcrInputNew->Release();

	// add alternative to results
	pxfres->Add(std::move(pexprLASJ));
}

// EOF
