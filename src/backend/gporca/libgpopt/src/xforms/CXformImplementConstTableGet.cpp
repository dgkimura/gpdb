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
CXformImplementConstTableGet::Transform(
	gpos::pointer<CXformContext *> pxfctxt,
	gpos::pointer<CXformResult *> pxfres,
	gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::pointer<CLogicalConstTableGet *> popConstTableGet =
		gpos::dyn_cast<CLogicalConstTableGet>(pexpr->Pop());
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
	gpos::owner<CExpression *> pexprAlt = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CPhysicalConstTableGet(mp, std::move(pdrgpcoldesc),
												std::move(pdrgpdrgpdatum),
												std::move(pdrgpcrOutput)));

	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}


// EOF
