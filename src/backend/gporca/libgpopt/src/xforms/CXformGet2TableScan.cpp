//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CXformGet2TableScan.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformGet2TableScan.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CPhysicalTableScan.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformGet2TableScan::CXformGet2TableScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformGet2TableScan::CXformGet2TableScan(CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalGet(mp)))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformGet2TableScan::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformGet2TableScan::Exfp(CExpressionHandle &exprhdl) const
{
	gpos::pointer<CLogicalGet *> popGet =
		gpos::dyn_cast<CLogicalGet>(exprhdl.Pop());

	gpos::pointer<CTableDescriptor *> ptabdesc = popGet->Ptabdesc();
	if (ptabdesc->IsPartitioned())
	{
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGet2TableScan::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformGet2TableScan::Transform(gpos::pointer<CXformContext *> pxfctxt,
							   gpos::pointer<CXformResult *> pxfres,
							   gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::pointer<CLogicalGet *> popGet =
		gpos::dyn_cast<CLogicalGet>(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// create/extract components for alternative
	CName *pname = GPOS_NEW(mp) CName(mp, popGet->Name());

	gpos::owner<CTableDescriptor *> ptabdesc = popGet->Ptabdesc();
	ptabdesc->AddRef();

	CColRefArray *pdrgpcrOutput = popGet->PdrgpcrOutput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);

	pdrgpcrOutput->AddRef();

	// create alternative expression
	gpos::owner<CExpression *> pexprAlt = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CPhysicalTableScan(
							mp, pname, std::move(ptabdesc), pdrgpcrOutput));
	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}


// EOF
