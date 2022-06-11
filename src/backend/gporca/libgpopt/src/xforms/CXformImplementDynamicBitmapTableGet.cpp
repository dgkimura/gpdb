//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformImplementDynamicBitmapTableGet.cpp
//
//	@doc:
//		Implement DynamicBitmapTableGet
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementDynamicBitmapTableGet.h"

#include "gpos/common/owner.h"

#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalDynamicBitmapTableGet.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalDynamicBitmapTableScan.h"

using namespace gpopt;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementDynamicBitmapTableGet::CXformImplementDynamicBitmapTableGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementDynamicBitmapTableGet::CXformImplementDynamicBitmapTableGet(
	CMemoryPool *mp)
	:  // pattern
	  CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalDynamicBitmapTableGet(mp),
		  GPOS_NEW(mp)
			  CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // predicate tree
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CPatternLeaf(mp))  // bitmap index expression
		  ))
{
}

// compute xform promise for a given expression handle
CXform::EXformPromise
CXformImplementDynamicBitmapTableGet::Exfp(CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasSubquery(0) || exprhdl.DeriveHasSubquery(1))
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementDynamicBitmapTableGet::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementDynamicBitmapTableGet::Transform(
	gpos::pointer<CXformContext *> pxfctxt GPOS_ASSERTS_ONLY,
	gpos::pointer<CXformResult *> pxfres GPOS_UNUSED,
	gpos::pointer<CExpression *> pexpr GPOS_ASSERTS_ONLY) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::pointer<CLogicalDynamicBitmapTableGet *> popLogical =
		gpos::dyn_cast<CLogicalDynamicBitmapTableGet>(pexpr->Pop());

	gpos::owner<CTableDescriptor *> ptabdesc = popLogical->Ptabdesc();
	ptabdesc->AddRef();

	CName *pname = GPOS_NEW(mp) CName(mp, popLogical->Name());

	CColRefArray *pdrgpcrOutput = popLogical->PdrgpcrOutput();

	GPOS_ASSERT(nullptr != pdrgpcrOutput);
	pdrgpcrOutput->AddRef();

	gpos::owner<CColRef2dArray *> pdrgpdrgpcrPart =
		popLogical->PdrgpdrgpcrPart();
	pdrgpdrgpcrPart->AddRef();

	popLogical->GetPartitionMdids()->AddRef();
	popLogical->GetRootColMappingPerPart()->AddRef();

	gpos::owner<CPhysicalDynamicBitmapTableScan *> popPhysical =
		GPOS_NEW(mp) CPhysicalDynamicBitmapTableScan(
			mp, std::move(ptabdesc), pexpr->Pop()->UlOpId(), pname,
			popLogical->ScanId(), pdrgpcrOutput, std::move(pdrgpdrgpcrPart),
			popLogical->GetPartitionMdids(),
			popLogical->GetRootColMappingPerPart());

	CExpression *pexprCondition = (*pexpr)[0];
	CExpression *pexprIndexPath = (*pexpr)[1];
	pexprCondition->AddRef();
	pexprIndexPath->AddRef();

	gpos::owner<CExpression *> pexprPhysical = GPOS_NEW(mp)
		CExpression(mp, std::move(popPhysical), pexprCondition, pexprIndexPath);
	pxfres->Add(std::move(pexprPhysical));
}

// EOF
