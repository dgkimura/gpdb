//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformPushDownLeftOuterJoin.cpp
//
//	@doc:
//		Implementation of left outer join push down transformation
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformPushDownLeftOuterJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CPatternTree.h"
#include "gpopt/operators/CPredicateUtils.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformPushDownLeftOuterJoin::CXformPushDownLeftOuterJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformPushDownLeftOuterJoin::CXformPushDownLeftOuterJoin(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalLeftOuterJoin(mp),
			  GPOS_NEW(mp) CExpression	// outer child is an NAry-Join
			  (mp, GPOS_NEW(mp) CLogicalNAryJoin(mp),
			   GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp)),
			   GPOS_NEW(mp) CExpression(
				   mp,
				   GPOS_NEW(mp) CPatternTree(mp))  // NAry-join predicate tree
			   ),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // inner child is a leaf
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp))  // LOJ predicate tree
			  ))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformPushDownLeftOuterJoin::Exfp
//
//	@doc:
//		Xform promise
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformPushDownLeftOuterJoin::Exfp(CExpressionHandle &exprhdl) const
{
	CExpression *pexprScalar = exprhdl.PexprScalarRepChild(2);
	if (COperator::EopScalarConst == pexprScalar->Pop()->Eopid())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformPushDownLeftOuterJoin::Transform
//
//	@doc:
//		Transform LOJ whose outer child is an NAry-join to be a child
//		of NAry-join
//
//		Input:
//			LOJ (a=d)
//				|---NAry-Join (a=b) and (b=c)
//				|     |--A
//				|     |--B
//				|     +--C
//				+--D
//
//		Output:
//			  NAry-Join (a=b) and (b=c)
//				|--B
//				|--C
//				+--LOJ (a=d)
//					|--A
//					+--D
//
//---------------------------------------------------------------------------
void
CXformPushDownLeftOuterJoin::Transform(CXformContext *pxfctxt,
									   CXformResult *pxfres,
									   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	CExpression *pexprNAryJoin = (*pexpr)[0];
	CExpression *pexprLOJInnerChild = (*pexpr)[1];
	CExpression *pexprLOJScalarChild = (*pexpr)[2];

	CColRefSet *pcrsLOJUsed = pexprLOJScalarChild->DeriveUsedColumns();
	gpos::Ref<CExpressionArray> pdrgpexprLOJChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CExpressionArray> pdrgpexprNAryJoinChildren =
		GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG arity = pexprNAryJoin->Arity();
	CExpression *pexprNAryJoinScalarChild = (*pexprNAryJoin)[arity - 1];
	for (ULONG ul = 0; ul < arity - 1; ul++)
	{
		CExpression *pexprChild = (*pexprNAryJoin)[ul];
		CColRefSet *pcrsOutput = pexprChild->DeriveOutputColumns();
		;
		if (!pcrsOutput->IsDisjoint(pcrsLOJUsed))
		{
			pdrgpexprLOJChildren->Append(pexprChild);
		}
		else
		{
			pdrgpexprNAryJoinChildren->Append(pexprChild);
		}
	}

	if (pdrgpexprLOJChildren->Size() == 0)
	{
		// cannot create a valid LOJ; bail
		;
		;
		return;
	}

	gpos::Ref<CExpression> pexprLOJOuterChild = (*pdrgpexprLOJChildren)[0];
	if (1 < pdrgpexprLOJChildren->Size())
	{
		// collect all relations needed by LOJ outer side into a cross product,
		// normalization at the end of this function takes care of pushing NAry
		// join predicates down
		pdrgpexprLOJChildren->Append(
			CPredicateUtils::PexprConjunction(mp, nullptr /*pdrgpexpr*/));
		pexprLOJOuterChild = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalNAryJoin(mp), pdrgpexprLOJChildren);

		// reconstruct LOJ children and add only the created child
		pdrgpexprLOJChildren = GPOS_NEW(mp) CExpressionArray(mp);
		pdrgpexprLOJChildren->Append(pexprLOJOuterChild);
	}

	// continue with rest of LOJ inner and scalar children
	;
	pdrgpexprLOJChildren->Append(pexprLOJInnerChild);
	;
	pdrgpexprLOJChildren->Append(pexprLOJScalarChild);

	// build new LOJ
	gpos::Ref<CExpression> pexprLOJNew = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalLeftOuterJoin(mp), pdrgpexprLOJChildren);

	// add new NAry join children
	pdrgpexprNAryJoinChildren->Append(pexprLOJNew);
	;
	pdrgpexprNAryJoinChildren->Append(pexprNAryJoinScalarChild);

	if (3 > pdrgpexprNAryJoinChildren->Size())
	{
		// xform must generate a valid NAry-join expression
		// for example, in the following case we end-up with the same input
		// expression, which should be avoided:
		//
		//	Input:
		//
		//    LOJ (a=c) and (b=c)
		//     |--NAry-Join (a=b)
		//     |   |--A
		//     |   +--B
		//     +--C
		//
		//	Output:
		//
		//	  NAry-Join (true)
		//      +--LOJ (a=c) and (b=c)
		//           |--NAry-Join (a=b)
		//           |    |--A
		//           |    +--B
		//           +--C

		;
		return;
	}

	// create new NAry join
	gpos::Ref<CExpression> pexprNAryJoinNew =
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalNAryJoin(mp),
								 std::move(pdrgpexprNAryJoinChildren));

	// normalize resulting expression and add it to xform results
	gpos::Ref<CExpression> pexprResult =
		CNormalizer::PexprNormalize(mp, pexprNAryJoinNew.get());
	;

	pxfres->Add(std::move(pexprResult));
}

// EOF
