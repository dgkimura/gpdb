//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CXformJoinCommutativity.cpp
//
//	@doc:
//		Implementation of commutativity transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformJoinCommutativity.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/search/CGroupExpression.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformJoinCommutativity::CXformJoinCommutativity
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformJoinCommutativity::CXformJoinCommutativity(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalInnerJoin(mp),
			  GPOS_NEW(mp)
				  CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // left child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // right child
			  GPOS_NEW(mp)
				  CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)))  // predicate
	  )
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformJoinCommutativity::FCompatible
//
//	@doc:
//		Compatibility function for join commutativity
//
//---------------------------------------------------------------------------
BOOL
CXformJoinCommutativity::FCompatible(CXform::EXformId exfid)
{
	BOOL fCompatible = true;

	switch (exfid)
	{
		case CXform::ExfJoinCommutativity:
			fCompatible = false;
			break;
		default:
			fCompatible = true;
	}

	return fCompatible;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformJoinCommutativity::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformJoinCommutativity::Transform(gpos::Ref<CXformContext> pxfctxt,
								   gpos::Ref<CXformResult> pxfres,
								   gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), SharedFromThis(), pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	gpos::Ref<CExpression> pexprLeft = (*pexpr)[0];
	gpos::Ref<CExpression> pexprRight = (*pexpr)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[2];

	// addref children
	;
	;
	;

	// assemble transformed expression
	gpos::Ref<CExpression> pexprAlt =
		CUtils::PexprLogicalJoin<CLogicalInnerJoin>(mp, pexprRight, pexprLeft,
													pexprScalar);

	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}

// EOF
