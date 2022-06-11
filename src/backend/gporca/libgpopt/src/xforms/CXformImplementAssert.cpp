//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformImplementAssert.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementAssert.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalAssert.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalAssert.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementAssert::CXformImplementAssert
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementAssert::CXformImplementAssert(CMemoryPool *mp)
	:  // pattern
	  CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalAssert(mp),
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // relational child
		  GPOS_NEW(mp)
			  CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))  // predicate
		  ))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementAssert::Exfp
//
//	@doc:
//		Compute xform promise level for a given expression handle;
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformImplementAssert::Exfp(CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasSubquery(1))
	{
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementAssert::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementAssert::Transform(gpos::pointer<CXformContext *> pxfctxt,
								 gpos::pointer<CXformResult *> pxfres,
								 gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	gpos::pointer<CLogicalAssert *> popAssert =
		gpos::dyn_cast<CLogicalAssert>(pexpr->Pop());
	CExpression *pexprRelational = (*pexpr)[0];
	CExpression *pexprScalar = (*pexpr)[1];
	CException *pexc = popAssert->Pexc();

	// addref all children
	pexprRelational->AddRef();
	pexprScalar->AddRef();

	// assemble physical operator
	gpos::owner<CPhysicalAssert *> popPhysicalAssert =
		GPOS_NEW(mp) CPhysicalAssert(
			mp, GPOS_NEW(mp) CException(pexc->Major(), pexc->Minor(),
										pexc->Filename(), pexc->Line()));

	gpos::owner<CExpression *> pexprAssert = GPOS_NEW(mp) CExpression(
		mp, std::move(popPhysicalAssert), pexprRelational, pexprScalar);

	// add alternative to results
	pxfres->Add(std::move(pexprAssert));
}


// EOF
