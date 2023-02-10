//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformSplitGbAgg.cpp
//
//	@doc:
//		Implementation of the splitting of an aggregate into a pair of
//		local and global aggregate
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformSplitGbAgg.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefComputed.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternMultiTree.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarValuesList.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/IMDAggregate.h"

using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAgg::CXformSplitGbAgg
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformSplitGbAgg::CXformSplitGbAgg(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalGbAgg(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // relational child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp))  // scalar project list
			  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAgg::CXformSplitGbAgg
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformSplitGbAgg::CXformSplitGbAgg(gpos::Ref<CExpression> pexprPattern)
	: CXformExploration(std::move(pexprPattern))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAgg::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformSplitGbAgg::Exfp(CExpressionHandle &exprhdl) const
{
	// do not split aggregate if it is a local aggregate, has distinct aggs, has outer references,
	// or return types of Agg functions are ambiguous
	if (!gpos::dyn_cast<CLogicalGbAgg>(exprhdl.Pop())->FGlobal() ||
		0 < exprhdl.DeriveTotalDistinctAggs(1) ||
		0 < exprhdl.DeriveOuterReferences()->Size() ||
		nullptr == exprhdl.PexprScalarExactChild(1) ||
		CXformUtils::FHasAmbiguousType(
			exprhdl.PexprScalarExactChild(1 /*child_index*/),
			COptCtxt::PoctxtFromTLS()->Pmda()))
	{
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAgg::Transform
//
//	@doc:
//		Actual transformation to expand a global aggregate into a pair of
//		local and global aggregate
//
//---------------------------------------------------------------------------
void
CXformSplitGbAgg::Transform(gpos::Ref<CXformContext> pxfctxt,
							gpos::Ref<CXformResult> pxfres,
							gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::Ref<CLogicalGbAgg> popAgg =
		gpos::dyn_cast<CLogicalGbAgg>(pexpr->Pop());

	// extract components
	gpos::Ref<CExpression> pexprRelational = (*pexpr)[0];
	gpos::Ref<CExpression> pexprProjectList = (*pexpr)[1];

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

	gpos::Ref<CColRefArray> colref_array = popAgg->Pdrgpcr();

	;
	gpos::Ref<CColRefArray> pdrgpcrLocal = colref_array;

	;
	gpos::Ref<CColRefArray> pdrgpcrGlobal = colref_array;

	CColRefArray *pdrgpcrMinimal = popAgg->PdrgpcrMinimal();
	if (nullptr != pdrgpcrMinimal)
	{
		// addref minimal grouping columns twice to be used in local and global aggregate
		;
		;
	}

	gpos::Ref<CExpression> local_expr = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(mp, std::move(pdrgpcrLocal), pdrgpcrMinimal,
								   COperator::EgbaggtypeLocal /*egbaggtype*/
								   ),
		pexprRelational, std::move(pexprProjectListLocal));

	gpos::Ref<CExpression> pexprGlobal = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(mp, std::move(pdrgpcrGlobal), pdrgpcrMinimal,
								   COperator::EgbaggtypeGlobal /*egbaggtype*/
								   ),
		std::move(local_expr), std::move(pexprProjectListGlobal));

	pxfres->Add(std::move(pexprGlobal));
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAgg::PopulateLocalGlobalProjectList
//
//	@doc:
//		Populate the local or global project list from the input project list
//
//---------------------------------------------------------------------------
void
CXformSplitGbAgg::PopulateLocalGlobalProjectList(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprProjList,
	gpos::Ref<CExpression> *ppexprProjListLocal,
	gpos::Ref<CExpression> *ppexprProjListGlobal)
{
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// list of project elements for the new local and global aggregates
	gpos::Ref<CExpressionArray> pdrgpexprProjElemLocal =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CExpressionArray> pdrgpexprProjElemGlobal =
		GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG arity = pexprProjList->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprProgElem = (*pexprProjList)[ul];
		gpos::Ref<CScalarProjectElement> popScPrEl =
			gpos::dyn_cast<CScalarProjectElement>(pexprProgElem->Pop());

		// get the scalar agg func
		gpos::Ref<CExpression> pexprAggFunc = (*pexprProgElem)[0];
		gpos::Ref<CScalarAggFunc> popScAggFunc =
			gpos::dyn_cast<CScalarAggFunc>(pexprAggFunc->Pop());

		;
		;
		gpos::Ref<CScalarAggFunc> popScAggFuncLocal = CUtils::PopAggFunc(
			mp, popScAggFunc->MDId(),
			GPOS_NEW(mp)
				CWStringConst(mp, popScAggFunc->PstrAggFunc()->GetBuffer()),
			popScAggFunc->IsDistinct(), EaggfuncstageLocal, /* fGlobal */
			true /* fSplit */, nullptr /* pmdidResolvedReturnType */,
			EaggfunckindNormal, popScAggFunc->GetArgTypes());

		;
		;
		gpos::Ref<CScalarAggFunc> popScAggFuncGlobal = CUtils::PopAggFunc(
			mp, popScAggFunc->MDId(),
			GPOS_NEW(mp)
				CWStringConst(mp, popScAggFunc->PstrAggFunc()->GetBuffer()),
			false /* is_distinct */, EaggfuncstageGlobal, /* fGlobal */
			true /* fSplit */, nullptr /* pmdidResolvedReturnType */,
			EaggfunckindNormal, popScAggFunc->GetArgTypes());

		// determine column reference for the new project element
		const IMDAggregate *pmdagg =
			md_accessor->RetrieveAgg(popScAggFunc->MDId());
		const IMDType *pmdtype =
			md_accessor->RetrieveType(pmdagg->GetIntermediateResultTypeMdid());
		CColRef *pcrLocal =
			col_factory->PcrCreate(pmdtype, default_type_modifier);
		CColRef *pcrGlobal = popScPrEl->Pcr();

		// create a new local aggregate function
		// create array of arguments for the aggregate function
		gpos::Ref<CExpressionArray> pdrgpexprAgg = pexprAggFunc->PdrgPexpr();

		;
		CExpressionArray *pdrgpexprLocal = pdrgpexprAgg;

		gpos::Ref<CExpression> pexprAggFuncLocal =
			GPOS_NEW(mp) CExpression(mp, popScAggFuncLocal, pdrgpexprLocal);

		// create a new global aggregate function adding the column reference of the
		// intermediate result to the arguments of the global aggregate function
		gpos::Ref<CExpressionArray> fullargs =
			GPOS_NEW(mp) CExpressionArray(mp);

		gpos::Ref<CExpressionArray> pdrgpexprGlobal =
			GPOS_NEW(mp) CExpressionArray(mp);
		pdrgpexprGlobal->Append(CUtils::PexprScalarIdent(mp, pcrLocal));

		gpos::Ref<CScalarValuesList> l = GPOS_NEW(mp) CScalarValuesList(mp);
		gpos::Ref<CExpression> globall =
			GPOS_NEW(mp) CExpression(mp, l, pdrgpexprGlobal);
		fullargs->Append(globall);

		fullargs->Append(GPOS_NEW(mp)
							 CExpression(mp, GPOS_NEW(mp) CScalarValuesList(mp),
										 GPOS_NEW(mp) CExpressionArray(mp)));

		fullargs->Append(GPOS_NEW(mp)
							 CExpression(mp, GPOS_NEW(mp) CScalarValuesList(mp),
										 GPOS_NEW(mp) CExpressionArray(mp)));

		fullargs->Append(GPOS_NEW(mp)
							 CExpression(mp, GPOS_NEW(mp) CScalarValuesList(mp),
										 GPOS_NEW(mp) CExpressionArray(mp)));

		gpos::Ref<CExpression> pexprAggFuncGlobal =
			GPOS_NEW(mp) CExpression(mp, popScAggFuncGlobal, fullargs);

		// create new project elements for the aggregate functions
		gpos::Ref<CExpression> pexprProjElemLocal =
			CUtils::PexprScalarProjectElement(mp, pcrLocal, pexprAggFuncLocal);

		gpos::Ref<CExpression> pexprProjElemGlobal =
			CUtils::PexprScalarProjectElement(mp, pcrGlobal,
											  pexprAggFuncGlobal);

		pdrgpexprProjElemLocal->Append(pexprProjElemLocal);
		pdrgpexprProjElemGlobal->Append(pexprProjElemGlobal);
	}

	// create new project lists
	*ppexprProjListLocal =
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 std::move(pdrgpexprProjElemLocal));

	*ppexprProjListGlobal =
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 std::move(pdrgpexprProjElemGlobal));
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitGbAgg::FApplicable
//
//	@doc:
//		Check if we the transformation is applicable (no distinct qualified
//		aggregate (DQA)) present
//
//---------------------------------------------------------------------------
BOOL
CXformSplitGbAgg::FApplicable(gpos::Ref<CExpression> pexpr)
{
	const ULONG arity = pexpr->Arity();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprPrEl = (*pexpr)[ul];

		// get the scalar child of the project element
		gpos::Ref<CExpression> pexprAggFunc = (*pexprPrEl)[0];
		gpos::Ref<CScalarAggFunc> popScAggFunc =
			gpos::dyn_cast<CScalarAggFunc>(pexprAggFunc->Pop());

		if (popScAggFunc->IsDistinct() ||
			!md_accessor->RetrieveAgg(popScAggFunc->MDId())->IsSplittable())
		{
			return false;
		}
	}

	return true;
}

// EOF
