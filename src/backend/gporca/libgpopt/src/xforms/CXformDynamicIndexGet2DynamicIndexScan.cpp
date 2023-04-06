//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformDynamicIndexGet2DynamicIndexScan.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformDynamicIndexGet2DynamicIndexScan.h"

#include "gpos/base.h"

#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalDynamicIndexGet.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalDynamicIndexScan.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformDynamicIndexGet2DynamicIndexScan::CXformDynamicIndexGet2DynamicIndexScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformDynamicIndexGet2DynamicIndexScan::CXformDynamicIndexGet2DynamicIndexScan(
	CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalDynamicIndexGet(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp))  // index lookup predicate
			  ))
{
}

CXform::EXformPromise
CXformDynamicIndexGet2DynamicIndexScan::Exfp(CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasSubquery(0))
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDynamicIndexGet2DynamicIndexScan::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformDynamicIndexGet2DynamicIndexScan::Transform(
	gpos::Ref<CXformContext> pxfctxt GPOS_ASSERTS_ONLY,
	gpos::Ref<CXformResult> pxfres GPOS_UNUSED,
	gpos::Ref<CExpression> pexpr GPOS_ASSERTS_ONLY) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), SharedFromThis(), pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::Ref<CLogicalDynamicIndexGet> popIndexGet =
		CLogicalDynamicIndexGet::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// create/extract components for alternative
	CName *pname = GPOS_NEW(mp) CName(mp, popIndexGet->Name());
	GPOS_ASSERT(pname != nullptr);

	// extract components
	gpos::Ref<CExpression> pexprIndexCond = (*pexpr)[0];


	gpos::Ref<CTableDescriptor> ptabdesc = popIndexGet->Ptabdesc();


	gpos::Ref<CIndexDescriptor> pindexdesc = popIndexGet->Pindexdesc();


	gpos::Ref<CColRefArray> pdrgpcrOutput = popIndexGet->PdrgpcrOutput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);


	gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart = popIndexGet->PdrgpdrgpcrPart();


	gpos::Ref<COrderSpec> pos = popIndexGet->Pos();



	// create alternative expression
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp)
		CExpression(mp,
					GPOS_NEW(mp) CPhysicalDynamicIndexScan(
						mp, pindexdesc, ptabdesc, pexpr->Pop()->UlOpId(), pname,
						pdrgpcrOutput, popIndexGet->ScanId(), pdrgpdrgpcrPart,
						pos, popIndexGet->GetPartitionMdids(),
						popIndexGet->GetRootColMappingPerPart()),
					pexprIndexCond);
	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}


// EOF
