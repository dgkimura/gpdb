//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CXformImplementLimit.cpp
//
//	@doc:
//		Implementation of limit operator
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementLimit.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalLimit.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementLimit::CXformImplementLimit
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementLimit::CXformImplementLimit(CMemoryPool *mp)
	:  // pattern
	  CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalLimit(mp),
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // relational child
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // scalar child for offset
		  GPOS_NEW(mp) CExpression(
			  mp,
			  GPOS_NEW(mp) CPatternLeaf(mp))  // scalar child for number of rows

		  ))
{
}

CXform::EXformPromise
CXformImplementLimit::Exfp(CExpressionHandle &exprhdl) const
{
	// Although it is valid SQL for the limit/offset to be a subquery, Orca does
	// not support it
	if (exprhdl.DeriveHasSubquery(1) || exprhdl.DeriveHasSubquery(2))
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementLimit::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementLimit::Transform(gpos::pointer<CXformContext *> pxfctxt,
								gpos::pointer<CXformResult *> pxfres,
								gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	gpos::pointer<CLogicalLimit *> popLimit =
		gpos::dyn_cast<CLogicalLimit>(pexpr->Pop());
	CExpression *pexprRelational = (*pexpr)[0];
	CExpression *pexprScalarStart = (*pexpr)[1];
	CExpression *pexprScalarRows = (*pexpr)[2];
	gpos::owner<COrderSpec *> pos = popLimit->Pos();

	// addref all components
	pexprRelational->AddRef();
	pexprScalarStart->AddRef();
	pexprScalarRows->AddRef();
	popLimit->Pos()->AddRef();

	// assemble physical operator
	gpos::owner<CExpression *> pexprLimit = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalLimit(mp, std::move(pos), popLimit->FGlobal(),
									popLimit->FHasCount(),
									popLimit->IsTopLimitUnderDMLorCTAS()),
		pexprRelational, pexprScalarStart, pexprScalarRows);

	// add alternative to results
	pxfres->Add(std::move(pexprLimit));
}


// EOF
