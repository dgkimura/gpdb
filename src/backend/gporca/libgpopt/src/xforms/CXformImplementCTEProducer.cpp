//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformImplementCTEProducer.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementCTEProducer.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalCTEProducer.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalCTEProducer.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementCTEProducer::CXformImplementCTEProducer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementCTEProducer::CXformImplementCTEProducer(CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalCTEProducer(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementCTEProducer::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformImplementCTEProducer::Exfp(CExpressionHandle &  // exprhdl
) const
{
	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementCTEProducer::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementCTEProducer::Transform(gpos::pointer<CXformContext *> pxfctxt,
									  gpos::pointer<CXformResult *> pxfres,
									  gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::pointer<CLogicalCTEProducer *> popCTEProducer =
		gpos::dyn_cast<CLogicalCTEProducer>(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components for alternative
	ULONG id = popCTEProducer->UlCTEId();

	gpos::owner<CColRefArray *> colref_array = popCTEProducer->Pdrgpcr();
	colref_array->AddRef();

	// child of CTEProducer operator
	gpos::owner<CExpression *> pexprChild = (*pexpr)[0];
	pexprChild->AddRef();

	// create physical CTE Producer
	gpos::owner<CExpression *> pexprAlt = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CPhysicalCTEProducer(mp, id, std::move(colref_array)),
		std::move(pexprChild));

	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}

// EOF
