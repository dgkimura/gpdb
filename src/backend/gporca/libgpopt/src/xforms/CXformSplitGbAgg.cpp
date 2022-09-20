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
CXformSplitGbAgg::CXformSplitGbAgg(gpos::owner<CExpression *> pexprPattern)
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
CXformSplitGbAgg::Transform(gpos::pointer<CXformContext *> pxfctxt,
							gpos::pointer<CXformResult *> pxfres,
							gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::pointer<CLogicalGbAgg *> popAgg =
		gpos::dyn_cast<CLogicalGbAgg>(pexpr->Pop());

	// extract components
	CExpression *pexprRelational = (*pexpr)[0];
	gpos::pointer<CExpression *> pexprProjectList = (*pexpr)[1];

	// check if the transformation is applicable
	if (!FApplicable(pexprProjectList))
	{
		return;
	}

	pexprRelational->AddRef();

	gpos::owner<CExpression *> pexprProjectListLocal = nullptr;
	gpos::owner<CExpression *> pexprProjectListGlobal = nullptr;

	(void) PopulateLocalGlobalProjectList(
		mp, pexprProjectList, &pexprProjectListLocal, &pexprProjectListGlobal);

	GPOS_ASSERT(nullptr != pexprProjectListLocal &&
				nullptr != pexprProjectListLocal);

	gpos::owner<CColRefArray *> colref_array = popAgg->Pdrgpcr();

	colref_array->AddRef();
	gpos::owner<CColRefArray *> pdrgpcrLocal = colref_array;

	colref_array->AddRef();
	gpos::owner<CColRefArray *> pdrgpcrGlobal = colref_array;

	CColRefArray *pdrgpcrMinimal = popAgg->PdrgpcrMinimal();
	if (nullptr != pdrgpcrMinimal)
	{
		// addref minimal grouping columns twice to be used in local and global aggregate
		pdrgpcrMinimal->AddRef();
		pdrgpcrMinimal->AddRef();
	}

	gpos::owner<CExpression *> local_expr = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(mp, std::move(pdrgpcrLocal), pdrgpcrMinimal,
								   COperator::EgbaggtypeLocal /*egbaggtype*/
								   ),
		pexprRelational, std::move(pexprProjectListLocal));

	gpos::owner<CExpression *> pexprGlobal = GPOS_NEW(mp) CExpression(
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
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprProjList,
	gpos::owner<CExpression *> *ppexprProjListLocal,
	gpos::owner<CExpression *> *ppexprProjListGlobal)
{
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// list of project elements for the new local and global aggregates
	gpos::owner<CExpressionArray *> pdrgpexprProjElemLocal =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::owner<CExpressionArray *> pdrgpexprProjElemGlobal =
		GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG arity = pexprProjList->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::pointer<CExpression *> pexprProgElem = (*pexprProjList)[ul];
		gpos::pointer<CScalarProjectElement *> popScPrEl =
			gpos::dyn_cast<CScalarProjectElement>(pexprProgElem->Pop());

		// get the scalar agg func
		gpos::pointer<CExpression *> pexprAggFunc = (*pexprProgElem)[0];
		gpos::pointer<CScalarAggFunc *> popScAggFunc =
			gpos::dyn_cast<CScalarAggFunc>(pexprAggFunc->Pop());

		popScAggFunc->MDId()->AddRef();
		popScAggFunc->GetArgTypes()->AddRef();
		gpos::owner<CScalarAggFunc *> popScAggFuncLocal = CUtils::PopAggFunc(
			mp, popScAggFunc->MDId(),
			GPOS_NEW(mp)
				CWStringConst(mp, popScAggFunc->PstrAggFunc()->GetBuffer()),
			popScAggFunc->IsDistinct(), EaggfuncstageLocal, /* fGlobal */
			true /* fSplit */, nullptr /* pmdidResolvedReturnType */,
			EaggfunckindNormal, popScAggFunc->GetArgTypes());

		popScAggFunc->MDId()->AddRef();
		popScAggFunc->GetArgTypes()->AddRef();
		gpos::owner<CScalarAggFunc *> popScAggFuncGlobal = CUtils::PopAggFunc(
			mp, popScAggFunc->MDId(),
			GPOS_NEW(mp)
				CWStringConst(mp, popScAggFunc->PstrAggFunc()->GetBuffer()),
			false /* is_distinct */, EaggfuncstageGlobal, /* fGlobal */
			true /* fSplit */, nullptr /* pmdidResolvedReturnType */,
			EaggfunckindNormal, popScAggFunc->GetArgTypes());

		// determine column reference for the new project element
		gpos::pointer<const IMDAggregate *> pmdagg =
			md_accessor->RetrieveAgg(popScAggFunc->MDId());
		gpos::pointer<const IMDType *> pmdtype =
			md_accessor->RetrieveType(pmdagg->GetIntermediateResultTypeMdid());
		CColRef *pcrLocal =
			col_factory->PcrCreate(pmdtype, default_type_modifier);
		CColRef *pcrGlobal = popScPrEl->Pcr();

		// create a new local aggregate function
		// create array of arguments for the aggregate function
		gpos::owner<CExpressionArray *> pdrgpexprAgg =
			pexprAggFunc->PdrgPexpr();

		pdrgpexprAgg->AddRef();
		CExpressionArray *pdrgpexprLocal = pdrgpexprAgg;

		gpos::owner<CExpression *> pexprAggFuncLocal =
			GPOS_NEW(mp) CExpression(mp, popScAggFuncLocal, pdrgpexprLocal);

		// create a new global aggregate function adding the column reference of the
		// intermediate result to the arguments of the global aggregate function
		gpos::owner<CExpressionArray *> fullargs =
			GPOS_NEW(mp) CExpressionArray(mp);

		gpos::owner<CExpressionArray *> pdrgpexprGlobal =
			GPOS_NEW(mp) CExpressionArray(mp);
		pdrgpexprGlobal->Append(CUtils::PexprScalarIdent(mp, pcrLocal));

		gpos::owner<CScalarValuesList *> l = GPOS_NEW(mp) CScalarValuesList(mp);
		gpos::owner<CExpression *> globall =
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

		gpos::owner<CExpression *> pexprAggFuncGlobal =
			GPOS_NEW(mp) CExpression(mp, popScAggFuncGlobal, fullargs);

		// create new project elements for the aggregate functions
		gpos::owner<CExpression *> pexprProjElemLocal =
			CUtils::PexprScalarProjectElement(mp, pcrLocal, pexprAggFuncLocal);

		gpos::owner<CExpression *> pexprProjElemGlobal =
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
CXformSplitGbAgg::FApplicable(gpos::pointer<CExpression *> pexpr)
{
	const ULONG arity = pexpr->Arity();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::pointer<CExpression *> pexprPrEl = (*pexpr)[ul];

		// get the scalar child of the project element
		gpos::pointer<CExpression *> pexprAggFunc = (*pexprPrEl)[0];
		gpos::pointer<CScalarAggFunc *> popScAggFunc =
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
