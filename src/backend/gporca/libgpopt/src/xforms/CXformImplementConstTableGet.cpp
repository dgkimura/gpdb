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
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CPhysicalConstTableGet.h"

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
CXformImplementConstTableGet::Transform(CXformContext *pxfctxt,
										CXformResult *pxfres,
										CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CLogicalConstTableGet *popConstTableGet =
		CLogicalConstTableGet::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// create/extract components for alternative
	gpos::owner<CColumnDescriptorArray *> pdrgpcoldesc =
		popConstTableGet->Pdrgpcoldesc();
	pdrgpcoldesc->AddRef();

	gpos::owner<IDatum2dArray *> pdrgpdrgpdatum =
		popConstTableGet->Pdrgpdrgpdatum();
	pdrgpdrgpdatum->AddRef();

	gpos::owner<CColRefArray *> pdrgpcrOutput =
		popConstTableGet->PdrgpcrOutput();
	pdrgpcrOutput->AddRef();

	// create alternative expression
	gpos::owner<CExpression *> pexprAlt = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CPhysicalConstTableGet(
							mp, pdrgpcoldesc, pdrgpdrgpdatum, pdrgpcrOutput));

	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}


// EOF
