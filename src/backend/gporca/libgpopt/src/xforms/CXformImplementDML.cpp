//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformImplementDML.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementDML.h"

#include "gpos/base.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalDML.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalDML.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementDML::CXformImplementDML
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementDML::CXformImplementDML(CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalDML(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementDML::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformImplementDML::Exfp(CExpressionHandle &  // exprhdl
) const
{
	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementDML::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementDML::Transform(gpos::Ref<CXformContext> pxfctxt,
							  gpos::Ref<CXformResult> pxfres,
							  gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::Ref<CLogicalDML> popDML = CLogicalDML::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components for alternative

	CLogicalDML::EDMLOperator edmlop = popDML->Edmlop();

	gpos::Ref<CTableDescriptor> ptabdesc = popDML->Ptabdesc();
	ptabdesc->AddRef();

	CColRefArray *pdrgpcrSource = popDML->PdrgpcrSource();
	pdrgpcrSource->AddRef();
	gpos::Ref<CBitSet> pbsModified = popDML->PbsModified();
	pbsModified->AddRef();

	gpos::Ref<CColRef> pcrAction = popDML->PcrAction();
	gpos::Ref<CColRef> pcrCtid = popDML->PcrCtid();
	gpos::Ref<CColRef> pcrSegmentId = popDML->PcrSegmentId();
	BOOL fSplit = popDML->FSplit();

	// child of DML operator
	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
	pexprChild->AddRef();

	// create physical DML
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp)
			CPhysicalDML(mp, edmlop, ptabdesc, pdrgpcrSource, pbsModified,
						 pcrAction, pcrCtid, pcrSegmentId, fSplit),
		pexprChild);
	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}

// EOF
