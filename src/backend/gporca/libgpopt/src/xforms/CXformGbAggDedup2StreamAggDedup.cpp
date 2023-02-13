//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformGbAggDedup2StreamAggDedup.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformGbAggDedup2StreamAggDedup.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalGbAggDeduplicate.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalStreamAggDeduplicate.h"
#include "gpopt/xforms/CXformGbAgg2HashAgg.h"
#include "gpopt/xforms/CXformUtils.h"
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggDedup2StreamAggDedup::CXformGbAggDedup2StreamAggDedup
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformGbAggDedup2StreamAggDedup::CXformGbAggDedup2StreamAggDedup(
	CMemoryPool *mp)
	: CXformGbAgg2StreamAgg(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalGbAggDeduplicate(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggDedup2StreamAggDedup::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformGbAggDedup2StreamAggDedup::Transform(gpos::Ref<CXformContext> pxfctxt,
										   gpos::Ref<CXformResult> pxfres,
										   gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	gpos::Ref<CExpression> pexprRel = (*pexpr)[0];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[1];
	GPOS_ASSERT(0 == pexprScalar->Arity());

	// addref children
	;
	;

	gpos::Ref<CLogicalGbAggDeduplicate> popAggDedup =
		gpos::dyn_cast<CLogicalGbAggDeduplicate>(pexpr->Pop());
	gpos::Ref<CColRefArray> colref_array = popAggDedup->Pdrgpcr();
	;

	gpos::Ref<CColRefArray> pdrgpcrKeys = popAggDedup->PdrgpcrKeys();
	;

	// create alternative expression
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalStreamAggDeduplicate(
			mp, std::move(colref_array), popAggDedup->PdrgpcrMinimal(),
			popAggDedup->Egbaggtype(), std::move(pdrgpcrKeys),
			popAggDedup->FGeneratesDuplicates(),
			CXformUtils::FMultiStageAgg(pexpr),
			CXformUtils::FAggGenBySplitDQAXform(pexpr), popAggDedup->AggStage(),
			!CXformUtils::FLocalAggCreatedByEagerAggXform(pexpr)),
		pexprRel, pexprScalar);

	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}


// EOF
