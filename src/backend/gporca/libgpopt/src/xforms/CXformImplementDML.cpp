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
#include "gpos/common/owner.h"

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
CXformImplementDML::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
							  CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CLogicalDML *popDML = gpos::dyn_cast<CLogicalDML>(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components for alternative

	CLogicalDML::EDMLOperator edmlop = popDML->Edmlop();

	gpos::Ref<CTableDescriptor> ptabdesc = popDML->Ptabdesc();
	;

	gpos::Ref<CColRefArray> pdrgpcrSource = popDML->PdrgpcrSource();
	;
	gpos::Ref<CBitSet> pbsModified = popDML->PbsModified();
	;

	CColRef *pcrAction = popDML->PcrAction();
	CColRef *pcrTableOid = popDML->PcrTableOid();
	CColRef *pcrCtid = popDML->PcrCtid();
	CColRef *pcrSegmentId = popDML->PcrSegmentId();
	CColRef *pcrTupleOid = popDML->PcrTupleOid();

	// child of DML operator
	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
	;

	// create physical DML
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalDML(
			mp, edmlop, std::move(ptabdesc), std::move(pdrgpcrSource),
			std::move(pbsModified), pcrAction, pcrTableOid, pcrCtid,
			pcrSegmentId, pcrTupleOid),
		std::move(pexprChild));
	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}

// EOF
