//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformImplementBitmapTableGet.cpp
//
//	@doc:
//		Implement BitmapTableGet
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementBitmapTableGet.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalBitmapTableGet.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalBitmapTableScan.h"

using namespace gpopt;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementBitmapTableGet::CXformImplementBitmapTableGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementBitmapTableGet::CXformImplementBitmapTableGet(CMemoryPool *mp)
	:  // pattern
	  CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalBitmapTableGet(mp),
		  GPOS_NEW(mp)
			  CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // predicate tree
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CPatternLeaf(mp))  // bitmap index expression
		  ))
{
}

CXform::EXformPromise
CXformImplementBitmapTableGet::Exfp(CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasSubquery(0) || exprhdl.DeriveHasSubquery(1))
	{
		return CXform::ExfpNone;
	}


	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementBitmapTableGet::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementBitmapTableGet::Transform(gpos::Ref<CXformContext> pxfctxt,
										 gpos::Ref<CXformResult> pxfres,
										 gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::Ref<CLogicalBitmapTableGet> popLogical =
		CLogicalBitmapTableGet::PopConvert(pexpr->Pop());

	gpos::Ref<CTableDescriptor> ptabdesc = popLogical->Ptabdesc();
	ptabdesc->AddRef();

	CColRefArray *pdrgpcrOutput = popLogical->PdrgpcrOutput();
	pdrgpcrOutput->AddRef();

	gpos::Ref<CPhysicalBitmapTableScan> popPhysical = GPOS_NEW(mp)
		CPhysicalBitmapTableScan(mp, ptabdesc, pexpr->Pop()->UlOpId(),
								 GPOS_NEW(mp)
									 CName(mp, *popLogical->PnameTableAlias()),
								 pdrgpcrOutput);

	gpos::Ref<CExpression> pexprCondition = (*pexpr)[0];
	gpos::Ref<CExpression> pexprIndexPath = (*pexpr)[1];
	pexprCondition->AddRef();
	pexprIndexPath->AddRef();

	gpos::Ref<CExpression> pexprPhysical = GPOS_NEW(mp)
		CExpression(mp, popPhysical, pexprCondition, pexprIndexPath);
	pxfres->Add(pexprPhysical);
}

// EOF
