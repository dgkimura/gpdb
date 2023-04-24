//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformForeignGet2ForeignScan.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformForeignGet2ForeignScan.h"

#include "gpos/base.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalForeignGet.h"
#include "gpopt/operators/CPhysicalForeignScan.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformForeignGet2ForeignScan::CXformForeignGet2ForeignScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformForeignGet2ForeignScan::CXformForeignGet2ForeignScan(CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalForeignGet(mp)))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformForeignGet2ForeignScan::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformForeignGet2ForeignScan::Exfp(CExpressionHandle &	//exprhdl
) const
{
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformForeignGet2ForeignScan::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformForeignGet2ForeignScan::Transform(gpos::Ref<CXformContext> pxfctxt,
										gpos::Ref<CXformResult> pxfres,
										gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), SharedFromThis(), pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::Ref<CLogicalForeignGet> popGet =
		CLogicalForeignGet::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components for alternative
	CName *pname = GPOS_NEW(mp) CName(mp, popGet->Name());

	gpos::Ref<CTableDescriptor> ptabdesc = popGet->Ptabdesc();


	gpos::Ref<CColRefArray> pdrgpcrOutput = popGet->PdrgpcrOutput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);



	// create alternative expression
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalForeignScan(mp, pname, ptabdesc, pdrgpcrOutput));

	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}

// EOF
