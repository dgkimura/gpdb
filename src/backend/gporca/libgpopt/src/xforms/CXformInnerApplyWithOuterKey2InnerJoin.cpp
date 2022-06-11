//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformInnerApplyWithOuterKey2InnerJoin.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformInnerApplyWithOuterKey2InnerJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalInnerApply.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformInnerApplyWithOuterKey2InnerJoin::CXformInnerApplyWithOuterKey2InnerJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformInnerApplyWithOuterKey2InnerJoin::CXformInnerApplyWithOuterKey2InnerJoin(
	CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalInnerApply(mp),
			  GPOS_NEW(mp)
				  CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // left child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CLogicalGbAgg(mp),
				  GPOS_NEW(mp) CExpression(
					  mp,
					  GPOS_NEW(mp) CPatternTree(mp)),  // relational child of Gb
				  GPOS_NEW(mp) CExpression(
					  mp, GPOS_NEW(mp)
							  CPatternLeaf(mp))	 // scalar project list of Gb
				  ),							 // right child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp))  // Apply predicate
			  ))
{
}



//---------------------------------------------------------------------------
//	@function:
//		CXformInnerApplyWithOuterKey2InnerJoin::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformInnerApplyWithOuterKey2InnerJoin::Exfp(CExpressionHandle &exprhdl) const
{
	// check if outer child has key and inner child has outer references
	if (nullptr == exprhdl.DeriveKeyCollection(0) ||
		0 == exprhdl.DeriveOuterReferences(1)->Size())
	{
		return ExfpNone;
	}

	return ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformInnerApplyWithOuterKey2InnerJoin::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformInnerApplyWithOuterKey2InnerJoin::Transform(
	gpos::pointer<CXformContext *> pxfctxt,
	gpos::pointer<CXformResult *> pxfres,
	gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	CExpression *pexprOuter = (*pexpr)[0];
	gpos::pointer<CExpression *> pexprGb = (*pexpr)[1];
	CExpression *pexprScalar = (*pexpr)[2];

	if (0 < gpos::dyn_cast<CLogicalGbAgg>(pexprGb->Pop())->Pdrgpcr()->Size())
	{
		// xform is not applicable if inner Gb has grouping columns
		return;
	}

	if (CUtils::FHasSubqueryOrApply((*pexprGb)[0]))
	{
		// Subquery/Apply must be unnested before reaching here
		return;
	}

	// decorrelate Gb's relational child
	(*pexprGb)[0]->ResetDerivedProperties();
	gpos::owner<CExpression *> pexprInner = nullptr;
	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);
	if (!CDecorrelator::FProcess(mp, (*pexprGb)[0], false /*fEqualityOnly*/,
								 &pexprInner, pdrgpexpr,
								 pexprOuter->DeriveOutputColumns()))
	{
		pdrgpexpr->Release();
		return;
	}

	GPOS_ASSERT(nullptr != pexprInner);
	gpos::owner<CExpression *> pexprPredicate =
		CPredicateUtils::PexprConjunction(mp, std::move(pdrgpexpr));

	// join outer child with Gb's decorrelated child
	pexprOuter->AddRef();
	gpos::owner<CExpression *> pexprInnerJoin = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), pexprOuter,
					std::move(pexprInner), std::move(pexprPredicate));

	// create grouping columns from the output of outer child
	gpos::owner<CColRefArray *> pdrgpcrKey = nullptr;
	gpos::owner<CColRefArray *> colref_array =
		CUtils::PdrgpcrGroupingKey(mp, pexprOuter, &pdrgpcrKey);
	pdrgpcrKey->Release();	// key is not used here

	gpos::owner<CLogicalGbAgg *> popGbAgg =
		GPOS_NEW(mp) CLogicalGbAgg(mp, std::move(colref_array),
								   COperator::EgbaggtypeGlobal /*egbaggtype*/);
	gpos::owner<CExpression *> pexprPrjList = (*pexprGb)[1];
	pexprPrjList->AddRef();
	gpos::owner<CExpression *> pexprNewGb = GPOS_NEW(mp)
		CExpression(mp, std::move(popGbAgg), std::move(pexprInnerJoin),
					std::move(pexprPrjList));

	// add Apply predicate in a top Select node
	pexprScalar->AddRef();
	gpos::owner<CExpression *> pexprSelect =
		CUtils::PexprLogicalSelect(mp, std::move(pexprNewGb), pexprScalar);

	pxfres->Add(std::move(pexprSelect));
}


// EOF
