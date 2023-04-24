//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformGbAggDedup2HashAggDedup.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformGbAggDedup2HashAggDedup.h"

#include "gpos/base.h"

#include "gpopt/operators/CLogicalGbAggDeduplicate.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalHashAggDeduplicate.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggDedup2HashAggDedup::CXformGbAggDedup2HashAggDedup
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformGbAggDedup2HashAggDedup::CXformGbAggDedup2HashAggDedup(CMemoryPool *mp)
	: CXformGbAgg2HashAgg(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalGbAggDeduplicate(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggDedup2HashAggDedup::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformGbAggDedup2HashAggDedup::Transform(gpos::Ref<CXformContext> pxfctxt,
										 gpos::Ref<CXformResult> pxfres,
										 gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::Ref<CLogicalGbAggDeduplicate> popAggDedup =
		CLogicalGbAggDeduplicate::PopConvert(pexpr->Pop());
	CColRefArray *colref_array = popAggDedup->Pdrgpcr();
	colref_array->AddRef();

	CColRefArray *pdrgpcrKeys = popAggDedup->PdrgpcrKeys();
	pdrgpcrKeys->AddRef();

	// extract components
	gpos::Ref<CExpression> pexprRel = (*pexpr)[0];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[1];
	GPOS_ASSERT(0 == pexprScalar->Arity());

	// addref children
	pexprRel->AddRef();
	pexprScalar->AddRef();

	// create alternative expression
	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalHashAggDeduplicate(
			mp, colref_array, popAggDedup->PdrgpcrMinimal(),
			popAggDedup->Egbaggtype(), pdrgpcrKeys,
			popAggDedup->FGeneratesDuplicates(),
			CXformUtils::FMultiStageAgg(pexpr),
			CXformUtils::FAggGenBySplitDQAXform(pexpr), popAggDedup->AggStage(),
			!CXformUtils::FLocalAggCreatedByEagerAggXform(pexpr)),
		pexprRel, pexprScalar);

	// add alternative to transformation result
	pxfres->Add(pexprAlt);
}

// EOF
