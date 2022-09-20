//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformSimplifySubquery.cpp
//
//	@doc:
//		Simplify existential/quantified subqueries by transforming
//		into count(*) subqueries
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformSimplifySubquery.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/IMDScalarOp.h"

using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformSimplifySubquery::CXformSimplifySubquery
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformSimplifySubquery::CXformSimplifySubquery(
	gpos::Ref<CExpression> pexprPattern)
	: CXformExploration(std::move(pexprPattern))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSimplifySubquery::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//		subqueries must exist in scalar tree
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformSimplifySubquery::Exfp(CExpressionHandle &exprhdl) const
{
	// consider this transformation only if subqueries exist
	if (exprhdl.DeriveHasSubquery(1))
	{
		return CXform::ExfpHigh;
	}

	return CXform::ExfpNone;
	;
}



//---------------------------------------------------------------------------
//	@function:
//		CXformSimplifySubquery::FSimplifyQuantified
//
//	@doc:
//		Transform quantified subqueries to count(*) subqueries;
//		the function returns true if transformation succeeded
//
//---------------------------------------------------------------------------
BOOL
CXformSimplifySubquery::FSimplifyQuantified(
	CMemoryPool *mp, CExpression *pexprScalar,
	gpos::Ref<CExpression> *ppexprNewScalar)
{
	GPOS_ASSERT(CUtils::FQuantifiedSubquery(pexprScalar->Pop()));

	gpos::Ref<CExpression> pexprNewSubquery = nullptr;
	gpos::Ref<CExpression> pexprCmp = nullptr;
	CXformUtils::QuantifiedToAgg(mp, pexprScalar, &pexprNewSubquery, &pexprCmp);

	// create a comparison predicate involving subquery expression
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	;
	pdrgpexpr->Append(std::move(pexprNewSubquery));
	pdrgpexpr->Append((*pexprCmp)[1]);
	;

	*ppexprNewScalar =
		GPOS_NEW(mp) CExpression(mp, pexprCmp->Pop(), std::move(pdrgpexpr));
	;

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSimplifySubquery::FSimplifyExistential
//
//	@doc:
//		Transform existential subqueries to count(*) subqueries;
//		the function returns true if transformation succeeded
//
//---------------------------------------------------------------------------
BOOL
CXformSimplifySubquery::FSimplifyExistential(
	CMemoryPool *mp, CExpression *pexprScalar,
	gpos::Ref<CExpression> *ppexprNewScalar)
{
	GPOS_ASSERT(CUtils::FExistentialSubquery(pexprScalar->Pop()));

	gpos::Ref<CExpression> pexprNewSubquery = nullptr;
	gpos::Ref<CExpression> pexprCmp = nullptr;
	CXformUtils::ExistentialToAgg(mp, pexprScalar, &pexprNewSubquery,
								  &pexprCmp);

	// create a comparison predicate involving subquery expression
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	;
	pdrgpexpr->Append(std::move(pexprNewSubquery));
	pdrgpexpr->Append((*pexprCmp)[1]);
	;

	*ppexprNewScalar =
		GPOS_NEW(mp) CExpression(mp, pexprCmp->Pop(), std::move(pdrgpexpr));
	;

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSimplifySubquery::FSimplify
//
//	@doc:
//		Transform existential/quantified subqueries to count(*) subqueries;
//		the function returns true if transformation succeeded
//
//---------------------------------------------------------------------------
BOOL
CXformSimplifySubquery::FSimplifySubqueryRecursive(
	CMemoryPool *mp, CExpression *pexprScalar,
	gpos::Ref<CExpression> *ppexprNewScalar, FnSimplify *pfnsimplify,
	FnMatch *pfnmatch)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprScalar);

	if (pfnmatch(pexprScalar->Pop()))
	{
		return pfnsimplify(mp, pexprScalar, ppexprNewScalar);
	}

	// for all other types of subqueries, or if no other subqueries are
	// below this point, we add-ref root node and return immediately
	if (CUtils::FSubquery(pexprScalar->Pop()) ||
		!pexprScalar->DeriveHasSubquery())
	{
		;
		*ppexprNewScalar = pexprScalar;

		return true;
	}

	// otherwise, recursively process children
	const ULONG arity = pexprScalar->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	BOOL fSuccess = true;
	for (ULONG ul = 0; fSuccess && ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = nullptr;
		fSuccess = FSimplifySubqueryRecursive(
			mp, (*pexprScalar)[ul], &pexprChild, pfnsimplify, pfnmatch);
		if (fSuccess)
		{
			pdrgpexprChildren->Append(pexprChild);
		}
		else
		{
			;
		}
	}

	if (fSuccess)
	{
		gpos::Ref<COperator> pop = pexprScalar->Pop();
		;
		*ppexprNewScalar = GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
	}
	else
	{
		;
	}

	return fSuccess;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSimplifySubquery::Transform
//
//	@doc:
//		Actual transformation to simplify subquery expression
//
//---------------------------------------------------------------------------
void
CXformSimplifySubquery::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
								  CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	gpos::Ref<CExpression> pexprResult;
	pexprResult = FSimplifySubquery(mp, pexpr, FSimplifyExistential,
									CUtils::FExistentialSubquery);
	if (nullptr != pexprResult)
	{
		pxfres->Add(pexprResult);
	}

	pexprResult = FSimplifySubquery(mp, pexpr, FSimplifyQuantified,
									CUtils::FQuantifiedSubquery);
	if (nullptr != pexprResult)
	{
		pxfres->Add(pexprResult);

		// the last entry is used to replace existential subqueries with count(*)
		// after quantified subqueries have been replaced in the input expression
		pexprResult =
			FSimplifySubquery(mp, pexprResult.get(), FSimplifyExistential,
							  CUtils::FExistentialSubquery);
		if (nullptr != pexprResult)
		{
			pxfres->Add(std::move(pexprResult));
		}
	}
}

gpos::Ref<CExpression>
CXformSimplifySubquery::FSimplifySubquery(CMemoryPool *mp,
										  CExpression *pexprInput,
										  FnSimplify *pfnsimplify,
										  FnMatch *pfnmatch)
{
	CExpression *pexprOuter = (*pexprInput)[0];
	CExpression *pexprScalar = (*pexprInput)[1];
	gpos::Ref<CExpression> pexprNewScalar = nullptr;

	if (!FSimplifySubqueryRecursive(mp, pexprScalar, &pexprNewScalar,
									pfnsimplify, pfnmatch))
	{
		;
		return nullptr;
	}

	;
	gpos::Ref<CExpression> pexprResult = nullptr;
	if (COperator::EopLogicalSelect == pexprInput->Pop()->Eopid())
	{
		pexprResult =
			CUtils::PexprLogicalSelect(mp, pexprOuter, pexprNewScalar);
	}
	else
	{
		GPOS_ASSERT(COperator::EopLogicalProject == pexprInput->Pop()->Eopid());

		pexprResult = CUtils::PexprLogicalProject(
			mp, pexprOuter, pexprNewScalar, false /*fNewComputedCol*/);
	}

	// normalize resulting expression
	gpos::Ref<CExpression> pexprNormalized =
		CNormalizer::PexprNormalize(mp, pexprResult.get());
	;

	return pexprNormalized;
}


// EOF
