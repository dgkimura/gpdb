//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CXformGbAgg2StreamAgg.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformGbAgg2StreamAgg.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalStreamAgg.h"
#include "gpopt/xforms/CXformGbAgg2HashAgg.h"
#include "gpopt/xforms/CXformUtils.h"
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAgg2StreamAgg::CXformGbAgg2StreamAgg
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformGbAgg2StreamAgg::CXformGbAgg2StreamAgg(CMemoryPool *mp)
	: CXformImplementation(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalGbAgg(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAgg2StreamAgg::CXformGbAgg2StreamAgg
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformGbAgg2StreamAgg::CXformGbAgg2StreamAgg(
	gpos::owner<CExpression *> pexprPattern)
	: CXformImplementation(std::move(pexprPattern))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformGbAgg2StreamAgg::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//		grouping columns must be non-empty
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformGbAgg2StreamAgg::Exfp(CExpressionHandle &exprhdl) const
{
	gpos::pointer<CLogicalGbAgg *> popAgg =
		gpos::dyn_cast<CLogicalGbAgg>(exprhdl.Pop());
	if (0 == popAgg->Pdrgpcr()->Size() ||
		!CUtils::FComparisonPossible(popAgg->Pdrgpcr(), IMDType::EcmptL) ||
		exprhdl.DeriveHasSubquery(1))
	{
		// no grouping columns, or no sort operators are available for grouping columns, or
		// agg functions use subquery arguments
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAgg2StreamAgg::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformGbAgg2StreamAgg::Transform(gpos::pointer<CXformContext *> pxfctxt,
								 gpos::pointer<CXformResult *> pxfres,
								 gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));
	gpos::pointer<CLogicalGbAgg *> popAgg =
		gpos::dyn_cast<CLogicalGbAgg>(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::owner<CColRefArray *> colref_array = popAgg->Pdrgpcr();
	colref_array->AddRef();

	// extract components
	CExpression *pexprRel = (*pexpr)[0];
	CExpression *pexprScalar = (*pexpr)[1];

	// addref children
	pexprRel->AddRef();
	pexprScalar->AddRef();

	CColRefArray *pdrgpcrArgDQA = popAgg->PdrgpcrArgDQA();
	if (pdrgpcrArgDQA != nullptr && 0 != pdrgpcrArgDQA->Size())
	{
		pdrgpcrArgDQA->AddRef();
	}

	// create alternative expression
	gpos::owner<CExpression *> pexprAlt = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalStreamAgg(
			mp, std::move(colref_array), popAgg->PdrgpcrMinimal(),
			popAgg->Egbaggtype(), popAgg->FGeneratesDuplicates(), pdrgpcrArgDQA,
			CXformUtils::FMultiStageAgg(pexpr),
			CXformUtils::FAggGenBySplitDQAXform(pexpr), popAgg->AggStage(),
			!CXformUtils::FLocalAggCreatedByEagerAggXform(pexpr)),
		pexprRel, pexprScalar);

	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}


// EOF
