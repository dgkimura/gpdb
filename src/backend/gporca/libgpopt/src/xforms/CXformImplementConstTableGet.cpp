//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformImplementConstTableGet.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementConstTableGet.h"

#include "gpos/base.h"

#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CPhysicalConstTableGet.h"
#include "gpopt/search/CGroupExpression.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementConstTableGet::CXformImplementConstTableGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementConstTableGet::CXformImplementConstTableGet(CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalConstTableGet(mp)))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementConstTableGet::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementConstTableGet::Transform(gpos::Ref<CXformContext> pxfctxt,
										gpos::Ref<CXformResult> pxfres,
										gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), SharedFromThis(), pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::Ref<CLogicalConstTableGet> popConstTableGet =
		CLogicalConstTableGet::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// create/extract components for alternative
	gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc =
		popConstTableGet->Pdrgpcoldesc();


	gpos::Ref<IDatum2dArray> pdrgpdrgpdatum =
		popConstTableGet->Pdrgpdrgpdatum();


	gpos::Ref<CColRefArray> pdrgpcrOutput = popConstTableGet->PdrgpcrOutput();


	// create alternative expression
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CPhysicalConstTableGet(
							mp, pdrgpcoldesc, pdrgpdrgpdatum, pdrgpcrOutput));

	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}


// EOF
