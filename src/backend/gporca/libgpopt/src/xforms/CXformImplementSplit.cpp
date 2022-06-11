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
#include "gpos/common/owner.h"

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
CXformImplementSplit::Transform(gpos::pointer<CXformContext *> pxfctxt,
								gpos::pointer<CXformResult *> pxfres,
								gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::pointer<CLogicalSplit *> popSplit =
		gpos::dyn_cast<CLogicalSplit>(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components for alternative
	gpos::owner<CColRefArray *> pdrgpcrDelete = popSplit->PdrgpcrDelete();
	pdrgpcrDelete->AddRef();

	gpos::owner<CColRefArray *> pdrgpcrInsert = popSplit->PdrgpcrInsert();
	pdrgpcrInsert->AddRef();

	CColRef *pcrAction = popSplit->PcrAction();
	CColRef *pcrCtid = popSplit->PcrCtid();
	CColRef *pcrSegmentId = popSplit->PcrSegmentId();
	CColRef *pcrTupleOid = popSplit->PcrTupleOid();

	// child of Split operator
	CExpression *pexprChild = (*pexpr)[0];
	CExpression *pexprProjList = (*pexpr)[1];
	pexprChild->AddRef();
	pexprProjList->AddRef();

	// create physical Split
	gpos::owner<CExpression *> pexprAlt = GPOS_NEW(mp)
		CExpression(mp,
					GPOS_NEW(mp) CPhysicalSplit(
						mp, std::move(pdrgpcrDelete), std::move(pdrgpcrInsert),
						pcrCtid, pcrSegmentId, pcrAction, pcrTupleOid),
					pexprChild, pexprProjList);
	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}

// EOF
