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
#include "gpos/common/owner.h"

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
	gpos::pointer<CXformContext *> pxfctxt GPOS_ASSERTS_ONLY,
	gpos::pointer<CXformResult *> pxfres GPOS_UNUSED,
	gpos::pointer<CExpression *> pexpr GPOS_ASSERTS_ONLY) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::pointer<CLogicalDynamicIndexGet *> popIndexGet =
		gpos::dyn_cast<CLogicalDynamicIndexGet>(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// create/extract components for alternative
	CName *pname = GPOS_NEW(mp) CName(mp, popIndexGet->Name());
	GPOS_ASSERT(pname != nullptr);

	// extract components
	gpos::owner<CExpression *> pexprIndexCond = (*pexpr)[0];
	pexprIndexCond->AddRef();

	gpos::owner<CTableDescriptor *> ptabdesc = popIndexGet->Ptabdesc();
	ptabdesc->AddRef();

	gpos::owner<CIndexDescriptor *> pindexdesc = popIndexGet->Pindexdesc();
	pindexdesc->AddRef();

	CColRefArray *pdrgpcrOutput = popIndexGet->PdrgpcrOutput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);
	pdrgpcrOutput->AddRef();

	gpos::owner<CColRef2dArray *> pdrgpdrgpcrPart =
		popIndexGet->PdrgpdrgpcrPart();
	pdrgpdrgpcrPart->AddRef();

	gpos::owner<COrderSpec *> pos = popIndexGet->Pos();
	pos->AddRef();

	popIndexGet->GetPartitionMdids()->AddRef();
	popIndexGet->GetRootColMappingPerPart()->AddRef();

	// create alternative expression
	gpos::owner<CExpression *> pexprAlt = GPOS_NEW(mp)
		CExpression(mp,
					GPOS_NEW(mp) CPhysicalDynamicIndexScan(
						mp, std::move(pindexdesc), std::move(ptabdesc),
						pexpr->Pop()->UlOpId(), pname, pdrgpcrOutput,
						popIndexGet->ScanId(), std::move(pdrgpdrgpcrPart),
						std::move(pos), popIndexGet->GetPartitionMdids(),
						popIndexGet->GetRootColMappingPerPart()),
					std::move(pexprIndexCond));
	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}


// EOF
