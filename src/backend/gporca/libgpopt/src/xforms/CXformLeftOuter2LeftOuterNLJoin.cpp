//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformLeftOuter2LeftOuterNLJoin.cpp
//
//	@doc:
//		Simplify Left Outer Join with constant false predicate
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftOuterNLJoin.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternTree.h"
#include "gpopt/xforms/CXformLeftOuter2LeftOuterNLJoin.h"

using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftOuter2LeftOuterNLJoin::CXformLeftOuter2LeftOuterNLJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformLeftOuter2LeftOuterNLJoin::CXformLeftOuter2LeftOuterNLJoin
	(
	CMemoryPool *mp
	)
	:
	CXformExploration
		(
		 // pattern
		GPOS_NEW(mp) CExpression
					(
					mp,
					GPOS_NEW(mp) CLogicalLeftOuterJoin(mp),
					GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)), // left child
					GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // right child
					GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))  // predicate tree
					)
		)
{}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftOuter2LeftOuterNLJoin::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformLeftOuter2LeftOuterNLJoin::Exfp
	(
	CExpressionHandle &exprhdl
	)
	const
{
	CExpression *pexprScalar = exprhdl.PexprScalarChild(2);
	if (COperator::EopScalarConst == pexprScalar->Pop()->Eopid())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftOuter2LeftOuterNLJoin::Transform
//
//	@doc:
//		Actual transformation to simplify left outer join
//
//---------------------------------------------------------------------------
void
CXformLeftOuter2LeftOuterNLJoin::Transform
	(
	CXformContext *pxfctxt,
	CXformResult *pxfres,
	CExpression *pexpr
	)
	const
{
	GPOS_ASSERT(NULL != pxfctxt);
	GPOS_ASSERT(NULL != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	CExpression *pexprOuter = (*pexpr)[0];
	CExpression *pexprInner = (*pexpr)[1];
	CExpression *pexprScalar = (*pexpr)[2];

	pexprOuter->AddRef();
	pexprInner->AddRef();
	pexprScalar->AddRef();
	pexprScalar->AddRef();
	CExpression *pexprResult = NULL;

	pexprResult =
		GPOS_NEW(mp) CExpression
			(
			mp,
			GPOS_NEW(mp) CLogicalLeftOuterNLJoin(mp),
			pexprOuter,
			CUtils::PexprSafeSelect(mp, pexprInner, pexprScalar),
			pexprScalar
			);

	pxfres->Add(pexprResult);
}

// EOF
