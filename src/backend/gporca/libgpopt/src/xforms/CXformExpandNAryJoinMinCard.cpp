//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformExpandNAryJoinMinCard.cpp
//
//	@doc:
//		Implementation of n-ary join expansion based on cardinality
//		of intermediate results
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformExpandNAryJoinMinCard.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/CPatternMultiTree.h"
#include "gpopt/operators/CPatternTree.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/xforms/CJoinOrderMinCard.h"
#include "gpopt/xforms/CXformUtils.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandNAryJoinMinCard::CXformExpandNAryJoinMinCard
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformExpandNAryJoinMinCard::CXformExpandNAryJoinMinCard(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalNAryJoin(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiTree(mp)),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandNAryJoinMinCard::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformExpandNAryJoinMinCard::Exfp(CExpressionHandle &exprhdl) const
{
	return CXformUtils::ExfpExpandJoinOrder(exprhdl, this);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandNAryJoinMinCard::Transform
//
//	@doc:
//		Actual transformation of n-ary join to cluster of inner joins
//
//---------------------------------------------------------------------------
void
CXformExpandNAryJoinMinCard::Transform(gpos::pointer<CXformContext *> pxfctxt,
									   gpos::pointer<CXformResult *> pxfres,
									   gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	const ULONG arity = pexpr->Arity();
	GPOS_ASSERT(arity >= 3);

	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity - 1; ul++)
	{
		gpos::owner<CExpression *> pexprChild = (*pexpr)[ul];
		pexprChild->AddRef();
		pdrgpexpr->Append(pexprChild);
	}

	gpos::pointer<CExpression *> pexprScalar = (*pexpr)[arity - 1];
	gpos::owner<CExpressionArray *> pdrgpexprPreds =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprScalar);

	// create a join order based on cardinality of intermediate results
	CJoinOrderMinCard jomc(mp, std::move(pdrgpexpr), std::move(pdrgpexprPreds));
	gpos::owner<CExpression *> pexprResult = jomc.PexprExpand();

	// normalize resulting expression
	gpos::owner<CExpression *> pexprNormalized =
		CNormalizer::PexprNormalize(mp, pexprResult);
	pexprResult->Release();
	pxfres->Add(std::move(pexprNormalized));
}

// EOF
