//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformSplitGbAggDedup.cpp
//
//	@doc:
//		Implementation of the splitting of a dedup aggregate into a pair of
//		local and global aggregates
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformSplitGbAggDedup.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefComputed.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAggDeduplicate.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "naucrates/md/IMDAggregate.h"

using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAggDedup::CXformSplitGbAggDedup
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformSplitGbAggDedup::CXformSplitGbAggDedup(CMemoryPool *mp)
	: CXformSplitGbAgg(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalGbAggDeduplicate(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // relational child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp))  // scalar project list
			  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAggDedup::Transform
//
//	@doc:
//		Actual transformation to expand a global aggregate into a pair of
//		local and global aggregate
//
//---------------------------------------------------------------------------
void
CXformSplitGbAggDedup::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
								 CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CLogicalGbAggDeduplicate *popAggDedup =
		gpos::dyn_cast<CLogicalGbAggDeduplicate>(pexpr->Pop());

	// extract components
	CExpression *pexprRelational = (*pexpr)[0];
	CExpression *pexprProjectList = (*pexpr)[1];

	// check if the transformation is applicable
	if (!FApplicable(pexprProjectList))
	{
		return;
	}

	;

	gpos::Ref<CExpression> pexprProjectListLocal = nullptr;
	gpos::Ref<CExpression> pexprProjectListGlobal = nullptr;

	(void) PopulateLocalGlobalProjectList(
		mp, pexprProjectList, &pexprProjectListLocal, &pexprProjectListGlobal);
	GPOS_ASSERT(nullptr != pexprProjectListLocal &&
				nullptr != pexprProjectListLocal);

	gpos::Ref<CColRefArray> colref_array = popAggDedup->Pdrgpcr();
	;
	;

	CColRefArray *pdrgpcrMinimal = popAggDedup->PdrgpcrMinimal();
	if (nullptr != pdrgpcrMinimal)
	{
		;
		;
	}

	gpos::Ref<CColRefArray> pdrgpcrKeys = popAggDedup->PdrgpcrKeys();
	;
	;

	gpos::Ref<CExpression> local_expr = GPOS_NEW(mp)
		CExpression(mp,
					GPOS_NEW(mp) CLogicalGbAggDeduplicate(
						mp, colref_array, pdrgpcrMinimal,
						COperator::EgbaggtypeLocal /*egbaggtype*/, pdrgpcrKeys),
					pexprRelational, std::move(pexprProjectListLocal));

	gpos::Ref<CExpression> pexprGlobal = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAggDeduplicate(
			mp, colref_array, pdrgpcrMinimal,
			COperator::EgbaggtypeGlobal /*egbaggtype*/, pdrgpcrKeys),
		std::move(local_expr), std::move(pexprProjectListGlobal));

	pxfres->Add(std::move(pexprGlobal));
}

// EOF
