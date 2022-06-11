//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformCollapseProject.cpp
//
//	@doc:
//		Implementation of the transform that collapses two cascaded project nodes
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformCollapseProject.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternTree.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformCollapseProject::CXformCollapseProject
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformCollapseProject::CXformCollapseProject(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalProject(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CLogicalProject(mp),
				  GPOS_NEW(mp) CExpression(
					  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // relational child
				  GPOS_NEW(mp) CExpression(
					  mp, GPOS_NEW(mp) CPatternTree(mp))  // scalar project list
				  ),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp))  // scalar project list
			  ))
{
}



//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformCollapseProject::Exfp(CExpressionHandle &	 //exprhdl
) const
{
	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformCollapseProject::Transform
//
//	@doc:
//		Actual transformation to collapse projects
//
//---------------------------------------------------------------------------
void
CXformCollapseProject::Transform(gpos::pointer<CXformContext *> pxfctxt,
								 gpos::pointer<CXformResult *> pxfres,
								 gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	gpos::owner<CExpression *> pexprCollapsed =
		CUtils::PexprCollapseProjects(mp, pexpr);

	if (nullptr != pexprCollapsed)
	{
		pxfres->Add(std::move(pexprCollapsed));
	}
}

// EOF
