//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformImplementSplit.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementSplit.h"

#include "gpos/base.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalSplit.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalSplit.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementSplit::CXformImplementSplit
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementSplit::CXformImplementSplit(CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalSplit(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementSplit::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformImplementSplit::Exfp(CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasSubquery(1))
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementSplit::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementSplit::Transform(gpos::Ref<CXformContext> pxfctxt,
								gpos::Ref<CXformResult> pxfres,
								gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::Ref<CLogicalSplit> popSplit = CLogicalSplit::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components for alternative
	CColRefArray *pdrgpcrDelete = popSplit->PdrgpcrDelete();
	pdrgpcrDelete->AddRef();

	CColRefArray *pdrgpcrInsert = popSplit->PdrgpcrInsert();
	pdrgpcrInsert->AddRef();

	gpos::Ref<CColRef> pcrAction = popSplit->PcrAction();
	gpos::Ref<CColRef> pcrCtid = popSplit->PcrCtid();
	gpos::Ref<CColRef> pcrSegmentId = popSplit->PcrSegmentId();

	// child of Split operator
	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
	gpos::Ref<CExpression> pexprProjList = (*pexpr)[1];
	pexprChild->AddRef();
	pexprProjList->AddRef();

	// create physical Split
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalSplit(mp, pdrgpcrDelete, pdrgpcrInsert, pcrCtid,
									pcrSegmentId, pcrAction),
		pexprChild, pexprProjList);
	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}

// EOF
