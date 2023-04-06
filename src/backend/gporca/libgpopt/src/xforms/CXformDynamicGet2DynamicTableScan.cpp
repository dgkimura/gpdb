//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformDynamicGet2DynamicTableScan.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformDynamicGet2DynamicTableScan.h"

#include "gpos/base.h"

#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalDynamicGet.h"
#include "gpopt/operators/CPhysicalDynamicTableScan.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformDynamicGet2DynamicTableScan::CXformDynamicGet2DynamicTableScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformDynamicGet2DynamicTableScan::CXformDynamicGet2DynamicTableScan(
	CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalDynamicGet(mp)))
{
}

// compute xform promise for a given expression handle
CXform::EXformPromise
CXformDynamicGet2DynamicTableScan::Exfp(CExpressionHandle &exprhdl) const
{
	gpos::Ref<CLogicalDynamicGet> popGet =
		CLogicalDynamicGet::PopConvert(exprhdl.Pop());
	// Do not run if contains foreign partitions, instead run CXformExpandDynamicGetWithForeignPartitions
	if (popGet->ContainsForeignParts())
	{
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDynamicGet2DynamicTableScan::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformDynamicGet2DynamicTableScan::Transform(gpos::Ref<CXformContext> pxfctxt,
											 gpos::Ref<CXformResult> pxfres,
											 gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), SharedFromThis(), pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::Ref<CLogicalDynamicGet> popGet =
		CLogicalDynamicGet::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// create/extract components for alternative
	CName *pname = GPOS_NEW(mp) CName(mp, popGet->Name());

	gpos::Ref<CTableDescriptor> ptabdesc = popGet->Ptabdesc();


	gpos::Ref<CColRefArray> pdrgpcrOutput = popGet->PdrgpcrOutput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);



	gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart = popGet->PdrgpdrgpcrPart();



	// create alternative expression
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CPhysicalDynamicTableScan(
				mp, pname, ptabdesc, popGet->UlOpId(), popGet->ScanId(),
				pdrgpcrOutput, pdrgpdrgpcrPart, popGet->GetPartitionMdids(),
				popGet->GetRootColMappingPerPart()));
	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}


// EOF
