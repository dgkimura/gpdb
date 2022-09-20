//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformLeftSemiJoin2CrossProduct.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformLeftSemiJoin2CrossProduct.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternTree.h"
#include "gpopt/xforms/CXformUtils.h"



using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiJoin2CrossProduct::CXformLeftSemiJoin2CrossProduct
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CXformLeftSemiJoin2CrossProduct::CXformLeftSemiJoin2CrossProduct(
	CMemoryPool *mp)
	:  // pattern
	  CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalLeftSemiJoin(mp),
		  GPOS_NEW(mp) CExpression(
			  mp,
			  GPOS_NEW(mp) CPatternTree(
				  mp)),	 // left child is a tree since we may need to push predicates down
		  GPOS_NEW(mp)
			  CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // right child
		  GPOS_NEW(mp) CExpression(
			  mp,
			  GPOS_NEW(mp) CPatternTree(
				  mp))	// predicate is a tree since we may need to do clean-up of scalar expression
		  ))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiJoin2CrossProduct::Exfp
//
//	@doc:
//		 Compute xform promise for a given expression handle
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformLeftSemiJoin2CrossProduct::Exfp(CExpressionHandle &exprhdl) const
{
	return CXformUtils::ExfpSemiJoin2CrossProduct(exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiJoin2CrossProduct::Transform
//
//	@doc:
//		Semi join whose join predicate does not use columns from join's inner
//		child is equivalent to a Cross Product between outer child (after
//		pushing join predicate), and one tuple from inner child
//
//---------------------------------------------------------------------------
void
CXformLeftSemiJoin2CrossProduct::Transform(CXformContext *pxfctxt,
										   CXformResult *pxfres,
										   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	CExpression *pexprOuter = (*pexpr)[0];
	CExpression *pexprInner = (*pexpr)[1];
	CExpression *pexprScalar = (*pexpr)[2];
	;
	;
	;

	// create a (limit 1) on top of inner child
	gpos::Ref<CExpression> pexprLimitOffset =
		CUtils::PexprScalarConstInt8(mp, 0 /*val*/);
	gpos::Ref<CExpression> pexprLimitCount =
		CUtils::PexprScalarConstInt8(mp, 1 /*val*/);
	gpos::Ref<COrderSpec> pos = GPOS_NEW(mp) COrderSpec(mp);
	gpos::Ref<CLogicalLimit> popLimit = GPOS_NEW(mp)
		CLogicalLimit(mp, std::move(pos), true /*fGlobal*/, true /*fHasCount*/,
					  false /*fNonRemovableLimit*/);
	gpos::Ref<CExpression> pexprLimit = GPOS_NEW(mp)
		CExpression(mp, std::move(popLimit), pexprInner,
					std::move(pexprLimitOffset), std::move(pexprLimitCount));

	// create cross product
	gpos::Ref<CExpression> pexprJoin =
		CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, pexprOuter, std::move(pexprLimit), pexprScalar);
	gpos::Ref<CExpression> pexprNormalized =
		CNormalizer::PexprNormalize(mp, pexprJoin.get());
	;

	pxfres->Add(std::move(pexprNormalized));
}


// EOF
