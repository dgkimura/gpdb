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
CXformDifferenceAll2LeftAntiSemiJoin::Transform(CXformContext *pxfctxt,
												CXformResult *pxfres,
												CExpression *pexpr) const
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

	CLogicalDifferenceAll *popDifferenceAll =
		gpos::dyn_cast<CLogicalDifferenceAll>(pexpr->Pop());
	CColRef2dArray *pdrgpdrgpcrInput = popDifferenceAll->PdrgpdrgpcrInput();

	gpos::Ref<CExpression> pexprLeftWindow =
		CXformUtils::PexprWindowWithRowNumber(mp, pexprLeftChild,
											  (*pdrgpdrgpcrInput)[0].get());
	gpos::Ref<CExpression> pexprRightWindow =
		CXformUtils::PexprWindowWithRowNumber(mp, pexprRightChild,
											  (*pdrgpdrgpcrInput)[1].get());

	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInputNew =
		GPOS_NEW(mp) CColRef2dArray(mp);
	gpos::Ref<CColRefArray> pdrgpcrLeftNew =
		CUtils::PdrgpcrExactCopy(mp, (*pdrgpdrgpcrInput)[0].get());
	pdrgpcrLeftNew->Append(CXformUtils::PcrProjectElement(
		pexprLeftWindow.get(), 0 /* row_number window function*/));

	gpos::Ref<CColRefArray> pdrgpcrRightNew =
		CUtils::PdrgpcrExactCopy(mp, (*pdrgpdrgpcrInput)[1].get());
	pdrgpcrRightNew->Append(CXformUtils::PcrProjectElement(
		pexprRightWindow.get(), 0 /* row_number window function*/));

	pdrgpdrgpcrInputNew->Append(std::move(pdrgpcrLeftNew));
	pdrgpdrgpcrInputNew->Append(std::move(pdrgpcrRightNew));

	// generate the scalar condition for the left anti-semi join
	gpos::Ref<CExpression> pexprScCond =
		CUtils::PexprConjINDFCond(mp, pdrgpdrgpcrInputNew.get());

	// assemble the new left anti-semi join logical operator
	gpos::Ref<CExpression> pexprLASJ = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalLeftAntiSemiJoin(mp),
					std::move(pexprLeftWindow), std::move(pexprRightWindow),
					std::move(pexprScCond));

	// clean up
	;

	// add alternative to results
	pxfres->Add(std::move(pexprLASJ));
}

// EOF
