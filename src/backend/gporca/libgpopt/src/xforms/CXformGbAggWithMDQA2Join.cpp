//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformGbAggWithMDQA2Join.cpp
//
//	@doc:
//		Transform a GbAgg with multiple distinct qualified aggregates (MDQAs)
//		to a join tree with single DQA leaves
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformGbAggWithMDQA2Join.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/memory/CAutoMemoryPool.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggWithMDQA2Join::CXformGbAggWithMDQA2Join
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformGbAggWithMDQA2Join::CXformGbAggWithMDQA2Join(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalGbAgg(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp)),  // relational child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp))  // scalar project list
			  ))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggWithMDQA2Join::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformGbAggWithMDQA2Join::Exfp(CExpressionHandle &exprhdl) const
{
	CAutoMemoryPool amp;

	gpos::pointer<CLogicalGbAgg *> popAgg =
		gpos::dyn_cast<CLogicalGbAgg>(exprhdl.Pop());

	if (COperator::EgbaggtypeGlobal == popAgg->Egbaggtype() &&
		exprhdl.DeriveHasMultipleDistinctAggs(1))
	{
		return CXform::ExfpHigh;
	}

	return CXform::ExfpNone;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggWithMDQA2Join::PexprMDQAs2Join
//
//	@doc:
//		Converts GbAgg with multiple distinct aggregates into a join of single
//		distinct aggregates,
//
//		distinct aggregates that share the same argument are grouped together
//		in one leaf of the generated join expression,
//
//		non-distinct aggregates are also grouped together in one leaf of the
//		generated join expression
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CXformGbAggWithMDQA2Join::PexprMDQAs2Join(CMemoryPool *mp,
										  gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(COperator::EopLogicalGbAgg == pexpr->Pop()->Eopid());
	GPOS_ASSERT((*pexpr)[1]->DeriveHasMultipleDistinctAggs());

	// extract components
	gpos::pointer<CExpression *> pexprChild = (*pexpr)[0];

	gpos::pointer<CColRefSet *> pcrsChildOutput =
		pexprChild->DeriveOutputColumns();
	gpos::owner<CColRefArray *> pdrgpcrChildOutput =
		pcrsChildOutput->Pdrgpcr(mp);

	// create a CTE producer based on child expression
	gpos::pointer<CCTEInfo *> pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	const ULONG ulCTEId = pcteinfo->next_id();
	(void) CXformUtils::PexprAddCTEProducer(mp, ulCTEId, pdrgpcrChildOutput,
											pexprChild);

	// create a CTE consumer with child output columns
	gpos::owner<CExpression *> pexprConsumer = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalCTEConsumer(
							mp, ulCTEId, std::move(pdrgpcrChildOutput)));
	pcteinfo->IncrementConsumers(ulCTEId);

	// finalize GbAgg expression by replacing its child with CTE consumer
	pexpr->Pop()->AddRef();
	(*pexpr)[1]->AddRef();
	gpos::owner<CExpression *> pexprGbAggWithConsumer = GPOS_NEW(mp)
		CExpression(mp, pexpr->Pop(), std::move(pexprConsumer), (*pexpr)[1]);

	gpos::owner<CExpression *> pexprJoinDQAs =
		CXformUtils::PexprGbAggOnCTEConsumer2Join(mp, pexprGbAggWithConsumer);
	GPOS_ASSERT(nullptr != pexprJoinDQAs);

	pexprGbAggWithConsumer->Release();

	return GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ulCTEId),
					std::move(pexprJoinDQAs));
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggWithMDQA2Join::PexprExpandMDQAs
//
//	@doc:
//		Expand GbAgg with multiple distinct aggregates into a join of single
//		distinct aggregates,
//		return NULL if expansion is not done
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CXformGbAggWithMDQA2Join::PexprExpandMDQAs(CMemoryPool *mp,
										   gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(COperator::EopLogicalGbAgg == pexpr->Pop()->Eopid());

	gpos::pointer<COperator *> pop = pexpr->Pop();
	if (gpos::dyn_cast<CLogicalGbAgg>(pop)->FGlobal())
	{
		BOOL fHasMultipleDistinctAggs =
			(*pexpr)[1]->DeriveHasMultipleDistinctAggs();
		if (fHasMultipleDistinctAggs)
		{
			gpos::owner<CExpression *> pexprExpanded =
				PexprMDQAs2Join(mp, pexpr);

			// recursively process the resulting expression
			gpos::owner<CExpression *> pexprResult =
				PexprTransform(mp, pexprExpanded);
			pexprExpanded->Release();

			return pexprResult;
		}
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggWithMDQA2Join::PexprTransform
//
//	@doc:
//		Main transformation driver
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CXformGbAggWithMDQA2Join::PexprTransform(CMemoryPool *mp,
										 gpos::pointer<CExpression *> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	COperator *pop = pexpr->Pop();
	if (COperator::EopLogicalGbAgg == pop->Eopid())
	{
		gpos::owner<CExpression *> pexprResult = PexprExpandMDQAs(mp, pexpr);
		if (nullptr != pexprResult)
		{
			return pexprResult;
		}
	}

	// recursively process child expressions
	const ULONG arity = pexpr->Arity();
	gpos::owner<CExpressionArray *> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::owner<CExpression *> pexprChild =
			PexprTransform(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	pop->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pop, std::move(pdrgpexprChildren));
}


//---------------------------------------------------------------------------
//	@function:
//		CXformGbAggWithMDQA2Join::Transform
//
//	@doc:
//		Actual transformation to expand multiple distinct qualified aggregates
//		(MDQAs) to a join tree with single DQA leaves
//
//---------------------------------------------------------------------------
void
CXformGbAggWithMDQA2Join::Transform(gpos::pointer<CXformContext *> pxfctxt,
									gpos::pointer<CXformResult *> pxfres,
									gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	gpos::owner<CExpression *> pexprResult = PexprTransform(mp, pexpr);
	if (nullptr != pexprResult)
	{
		pxfres->Add(std::move(pexprResult));
	}
}

BOOL
CXformGbAggWithMDQA2Join::IsApplyOnce()
{
	return true;
}
// EOF
