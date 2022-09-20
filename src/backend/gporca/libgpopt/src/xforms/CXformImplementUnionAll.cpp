//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformImplementUnionAll.cpp
//
//	@doc:
//		Implementation of union all operator
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementUnionAll.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/exception.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CPhysicalUnionAll.h"
#include "gpopt/operators/CPhysicalUnionAllFactory.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementUnionAll::CXformImplementUnionAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementUnionAll::CXformImplementUnionAll(CMemoryPool *mp)
	:  // pattern
	  CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalUnionAll(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementUnionAll::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementUnionAll::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
								   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	CLogicalUnionAll *popUnionAll =
		gpos::dyn_cast<CLogicalUnionAll>(pexpr->Pop());
	CPhysicalUnionAllFactory factory(popUnionAll);

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG arity = pexpr->Arity();

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		;
		pdrgpexpr->Append(pexprChild);
	}

	gpos::Ref<CPhysicalUnionAll> popPhysicalSerialUnionAll =
		factory.PopPhysicalUnionAll(mp, false);

	// assemble serial union physical operator
	gpos::Ref<CExpression> pexprSerialUnionAll =
		GPOS_NEW(mp) CExpression(mp, popPhysicalSerialUnionAll, pdrgpexpr);

	// add serial union alternative to results
	pxfres->Add(pexprSerialUnionAll);

	// parallel union alternative to the result if the GUC is on
	BOOL fParallel = GPOS_FTRACE(EopttraceEnableParallelAppend);

	if (fParallel)
	{
		gpos::Ref<CPhysicalUnionAll> popPhysicalParallelUnionAll =
			factory.PopPhysicalUnionAll(mp, true);

		;

		// assemble physical parallel operator
		gpos::Ref<CExpression> pexprParallelUnionAll = GPOS_NEW(mp)
			CExpression(mp, std::move(popPhysicalParallelUnionAll), pdrgpexpr);

		// add parallel union alternative to results
		pxfres->Add(std::move(pexprParallelUnionAll));
	}
}

// EOF
