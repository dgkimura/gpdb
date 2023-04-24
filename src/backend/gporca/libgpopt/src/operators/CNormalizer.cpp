//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2012 EMC Corp.
//
//	@filename:
//		CNormalizer.cpp
//
//	@doc:
//		Implementation of expression tree normalizer
//---------------------------------------------------------------------------

#include "gpopt/operators/CNormalizer.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogical.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterCorrelatedApply.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarNAryJoinPredList.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::FPushThruOuterChild
//
//	@doc:
//		Check if we should push predicates through expression's outer child
//
//
//---------------------------------------------------------------------------
BOOL
CNormalizer::FPushThruOuterChild(gpos::Ref<CExpression> pexprLogical)
{
	GPOS_ASSERT(nullptr != pexprLogical);

	COperator::EOperatorId op_id = pexprLogical->Pop()->Eopid();

	return COperator::EopLogicalLeftOuterJoin == op_id ||
		   COperator::EopLogicalLeftOuterApply == op_id ||
		   COperator::EopLogicalLeftOuterCorrelatedApply == op_id ||
		   CUtils::FLeftAntiSemiApply(pexprLogical->Pop()) ||
		   CUtils::FLeftSemiApply(pexprLogical->Pop());
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::FPushableThruSeqPrjChild
//
//	@doc:
//		Check if a predicate can be pushed through the child of a sequence
//		project expression
//
//---------------------------------------------------------------------------
BOOL
CNormalizer::FPushableThruSeqPrjChild(gpos::Ref<CExpression> pexprSeqPrj,
									  gpos::Ref<CExpression> pexprPred)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);
	GPOS_ASSERT(nullptr != pexprPred);
	GPOS_ASSERT(CLogical::EopLogicalSequenceProject ==
				pexprSeqPrj->Pop()->Eopid());

	gpos::Ref<CDistributionSpec> pds =
		CLogicalSequenceProject::PopConvert(pexprSeqPrj->Pop())->Pds();

	BOOL fPushable = false;
	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		GPOS_ASSERT(nullptr ==
					CDistributionSpecHashed::PdsConvert(pds)->PdshashedEquiv());
		CAutoMemoryPool amp;
		CMemoryPool *mp = amp.Pmp();
		gpos::Ref<CColRefSet> pcrsUsed = pexprPred->DeriveUsedColumns();
		gpos::Ref<CColRefSet> pcrsPartCols = CUtils::PcrsExtractColumns(
			mp, CDistributionSpecHashed::PdsConvert(pds)->Pdrgpexpr());
		if (pcrsPartCols->ContainsAll(pcrsUsed))
		{
			// predicate is pushable if used columns are included in partition-by expression
			fPushable = true;
		}
		pcrsPartCols->Release();
	}

	return fPushable;
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::FPushable
//
//	@doc:
//		Check if a predicate can be pushed through a logical expression
//
//
//---------------------------------------------------------------------------
BOOL
CNormalizer::FPushable(gpos::Ref<CExpression> pexprLogical,
					   gpos::Ref<CExpression> pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLogical);
	GPOS_ASSERT(nullptr != pexprPred);

	// do not push through volatile functions below an aggregate
	// volatile functions can potentially give different results when they
	// are called so we don't want aggregate to use volatile function result
	// while it processes each row
	if (COperator::EopLogicalGbAgg == pexprLogical->Pop()->Eopid() &&
		(CPredicateUtils::FContainsVolatileFunction(pexprPred)))
	{
		return false;
	}


	gpos::Ref<CColRefSet> pcrsUsed = pexprPred->DeriveUsedColumns();
	gpos::Ref<CColRefSet> pcrsOutput = pexprLogical->DeriveOutputColumns();

	//	In case of a Union or UnionAll the predicate might get pushed
	//	to multiple children In such cases we will end up with duplicate
	//	CTEProducers having the same cte_id.
	return pcrsOutput->ContainsAll(pcrsUsed) &&
		   !CUtils::FHasCTEAnchor(pexprPred);
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PexprRecursiveNormalize
//
//	@doc:
//		Call normalizer recursively on children array
//
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CNormalizer::PexprRecursiveNormalize(CMemoryPool *mp,
									 gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	const ULONG arity = pexpr->Arity();
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = PexprNormalize(mp, (*pexpr)[ul]);
		pdrgpexpr->Append(pexprChild);
	}

	gpos::Ref<COperator> pop = pexpr->Pop();
	pop->AddRef();

	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::SplitConjunct
//
//	@doc:
//		Split the given conjunct into pushable and unpushable predicates
//
//
//---------------------------------------------------------------------------
void
CNormalizer::SplitConjunct(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
						   gpos::Ref<CExpression> pexprConj,
						   CExpressionArray **ppdrgpexprPushable,
						   CExpressionArray **ppdrgpexprUnpushable)
{
	GPOS_ASSERT(pexpr->Pop()->FLogical());
	GPOS_ASSERT(pexprConj->Pop()->FScalar());
	GPOS_ASSERT(nullptr != ppdrgpexprPushable);
	GPOS_ASSERT(nullptr != ppdrgpexprUnpushable);

	// collect pushable predicates from given conjunct
	*ppdrgpexprPushable = GPOS_NEW(mp) CExpressionArray(mp);
	*ppdrgpexprUnpushable = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionArray *pdrgpexprConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprConj);
	const ULONG size = pdrgpexprConjuncts->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CExpression> pexprScalar = (*pdrgpexprConjuncts)[ul];
		pexprScalar->AddRef();
		if (FPushable(pexpr, pexprScalar))
		{
			(*ppdrgpexprPushable)->Append(pexprScalar);
		}
		else
		{
			(*ppdrgpexprUnpushable)->Append(pexprScalar);
		}
	}
	pdrgpexprConjuncts->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThruOuterChild
//
//	@doc:
//		Push scalar expression through left outer join children;
//		this only handles the case of a SELECT on top of LEFT OUTER JOIN;
//		pushing down join predicates is handled in PushThruJoin();
//		here, we push predicates of the top SELECT node through LEFT OUTER JOIN's
//		outer child
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThruOuterChild(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
								gpos::Ref<CExpression> pexprConj,
								gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(FPushThruOuterChild(pexpr));
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	if (0 == pexpr->Arity())
	{
		// end recursion early for leaf patterns extracted from memo
		pexpr->AddRef();
		pexprConj->AddRef();
		*ppexprResult = CUtils::PexprSafeSelect(mp, pexpr, pexprConj);

		return;
	}

	gpos::Ref<CExpression> pexprOuter = (*pexpr)[0];
	gpos::Ref<CExpression> pexprInner = (*pexpr)[1];
	gpos::Ref<CExpression> pexprPred = (*pexpr)[2];

	CExpressionArray *pdrgpexprPushable = nullptr;
	CExpressionArray *pdrgpexprUnpushable = nullptr;
	SplitConjunct(mp, pexprOuter, pexprConj, &pdrgpexprPushable,
				  &pdrgpexprUnpushable);

	if (0 < pdrgpexprPushable->Size())
	{
		pdrgpexprPushable->AddRef();
		gpos::Ref<CExpression> pexprNewConj =
			CPredicateUtils::PexprConjunction(mp, pdrgpexprPushable);

		// create a new select node on top of the outer child
		pexprOuter->AddRef();
		gpos::Ref<CExpression> pexprNewSelect = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprNewConj);

		// push predicate through the new select to create a new outer child
		gpos::Ref<CExpression> pexprNewOuter = nullptr;
		PushThru(mp, pexprNewSelect, pexprNewConj, &pexprNewOuter);
		pexprNewSelect->Release();

		// create a new outer join using the new outer child and the new inner child
		gpos::Ref<COperator> pop = pexpr->Pop();
		pop->AddRef();
		pexprInner->AddRef();
		pexprPred->AddRef();
		gpos::Ref<CExpression> pexprNew = GPOS_NEW(mp)
			CExpression(mp, pop, pexprNewOuter, pexprInner, pexprPred);

		// call push down predicates on the new outer join
		gpos::Ref<CExpression> pexprConstTrue =
			CUtils::PexprScalarConstBool(mp, true /*value*/);
		PushThru(mp, pexprNew, pexprConstTrue, ppexprResult);
		pexprConstTrue->Release();
		pexprNew->Release();
	}

	if (0 < pdrgpexprUnpushable->Size())
	{
		gpos::Ref<CExpression> pexprOuterJoin = pexpr;
		if (0 < pdrgpexprPushable->Size())
		{
			pexprOuterJoin = *ppexprResult;
			GPOS_ASSERT(nullptr != pexprOuterJoin);
		}

		// call push down on the outer join predicates
		gpos::Ref<CExpression> pexprNew = nullptr;
		gpos::Ref<CExpression> pexprConstTrue =
			CUtils::PexprScalarConstBool(mp, true /*value*/);
		PushThru(mp, pexprOuterJoin, pexprConstTrue, &pexprNew);
		if (pexprOuterJoin != pexpr)
		{
			pexprOuterJoin->Release();
		}
		pexprConstTrue->Release();

		// create a SELECT on top of the new outer join
		pdrgpexprUnpushable->AddRef();
		*ppexprResult = PexprSelect(mp, pexprNew, pdrgpexprUnpushable);
	}

	pdrgpexprPushable->Release();
	pdrgpexprUnpushable->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::FSimplifySelectOnOuterJoin
//
//	@doc:
//		A SELECT on top of LOJ, where SELECT's predicate is NULL-filtering and
//		uses columns from LOJ's inner child, is simplified as Inner-Join
//
//		Example:
//
//			select * from (select * from R left join S on r1=s1) as foo where foo.s1>0;
//
//			is converted to:
//
//			select * from R inner join S on r1=s1 and s1>0;
//
//
//
//---------------------------------------------------------------------------
BOOL
CNormalizer::FSimplifySelectOnOuterJoin(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprOuterJoin,
	gpos::Ref<CExpression> pexprPred,  // selection predicate
	gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopLogicalLeftOuterJoin ==
				pexprOuterJoin->Pop()->Eopid());
	GPOS_ASSERT(pexprPred->Pop()->FScalar());
	GPOS_ASSERT(nullptr != ppexprResult);

	if (0 == pexprOuterJoin->Arity())
	{
		// exit early for leaf patterns extracted from memo
		*ppexprResult = nullptr;
		return false;
	}

	gpos::Ref<CExpression> pexprOuterJoinOuterChild = (*pexprOuterJoin)[0];
	gpos::Ref<CExpression> pexprOuterJoinInnerChild = (*pexprOuterJoin)[1];
	gpos::Ref<CExpression> pexprOuterJoinPred = (*pexprOuterJoin)[2];

	gpos::Ref<CColRefSet> pcrsOutput =
		pexprOuterJoinInnerChild->DeriveOutputColumns();
	if (!GPOS_FTRACE(EopttraceDisableOuterJoin2InnerJoinRewrite) &&
		CPredicateUtils::FNullRejecting(mp, pexprPred, pcrsOutput))
	{
		// we have a predicate on top of LOJ that uses LOJ's inner child,
		// if the predicate filters-out nulls, we can add it to the join
		// predicate and turn LOJ into Inner-Join
		pexprOuterJoinOuterChild->AddRef();
		pexprOuterJoinInnerChild->AddRef();

		*ppexprResult = GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) CLogicalInnerJoin(mp),
						pexprOuterJoinOuterChild, pexprOuterJoinInnerChild,
						CPredicateUtils::PexprConjunction(mp, pexprPred,
														  pexprOuterJoinPred));

		return true;
	}

	// failed to convert LOJ to inner-join
	return false;
}

// A SELECT on top of FOJ, where SELECT's predicate is NULL-filtering and uses
// columns from FOJ's outer child, is simplified as a SELECT on top of a
// Left-Join
// Example:
//   select * from lhs full join rhs on (lhs.a=rhs.a) where lhs.a = 5;
// is converted to:
//   select * from lhs left join rhs on (lhs.a=rhs.a) where lhs.a = 5;;
BOOL
CNormalizer::FSimplifySelectOnFullJoin(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprFullJoin,
	gpos::Ref<CExpression> pexprPred,  // selection predicate
	gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopLogicalFullOuterJoin ==
				pexprFullJoin->Pop()->Eopid());
	GPOS_ASSERT(pexprPred->Pop()->FScalar());
	GPOS_ASSERT(nullptr != ppexprResult);

	if (0 == pexprFullJoin->Arity())
	{
		// exit early for leaf patterns extracted from memo
		*ppexprResult = nullptr;
		return false;
	}

	gpos::Ref<CExpression> pexprLeftChild = (*pexprFullJoin)[0];
	gpos::Ref<CExpression> pexprRightChild = (*pexprFullJoin)[1];
	gpos::Ref<CExpression> pexprJoinPred = (*pexprFullJoin)[2];

	gpos::Ref<CColRefSet> pcrsOutputLeftChild =
		pexprLeftChild->DeriveOutputColumns();

	if (CPredicateUtils::FNullRejecting(mp, pexprPred, pcrsOutputLeftChild))
	{
		// we have a predicate on top of FOJ that uses FOJ's outer child,
		// if the predicate filters-out nulls, we can convert the FOJ to LOJ
		pexprLeftChild->AddRef();
		pexprRightChild->AddRef();
		pexprJoinPred->AddRef();
		pexprPred->AddRef();

		*ppexprResult = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalSelect(mp),
			GPOS_NEW(mp)
				CExpression(mp, GPOS_NEW(mp) CLogicalLeftOuterJoin(mp),
							pexprLeftChild, pexprRightChild, pexprJoinPred),
			pexprPred);

		return true;
	}

	// failed to convert FOJ to LOJ
	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThruSelect
//
//	@doc:
//		Push a conjunct through a select
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThruSelect(CMemoryPool *mp, gpos::Ref<CExpression> pexprSelect,
							gpos::Ref<CExpression> pexprConj,
							gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	gpos::Ref<CExpression> pexprLogicalChild = (*pexprSelect)[0];
	gpos::Ref<CExpression> pexprScalarChild = (*pexprSelect)[1];
	gpos::Ref<CExpression> pexprPred =
		CPredicateUtils::PexprConjunction(mp, pexprScalarChild, pexprConj);

	if (CUtils::FScalarConstTrue(pexprPred))
	{
		pexprPred->Release();
		*ppexprResult = PexprNormalize(mp, pexprLogicalChild);

		return;
	}

	COperator::EOperatorId op_id = pexprLogicalChild->Pop()->Eopid();
	gpos::Ref<CExpression> pexprSimplified = nullptr;
	if (COperator::EopLogicalLeftOuterJoin == op_id &&
		FSimplifySelectOnOuterJoin(mp, pexprLogicalChild, pexprPred,
								   &pexprSimplified))
	{
		// simplification succeeded, normalize resulting expression
		*ppexprResult = PexprNormalize(mp, pexprSimplified);
		pexprPred->Release();
		pexprSimplified->Release();

		return;
	}
	if (COperator::EopLogicalFullOuterJoin == op_id &&
		FSimplifySelectOnFullJoin(mp, pexprLogicalChild, pexprPred,
								  &pexprSimplified))
	{
		// simplification succeeded, normalize resulting expression
		*ppexprResult = PexprNormalize(mp, pexprSimplified);
		pexprPred->Release();
		pexprSimplified->Release();

		return;
	}

	if (FPushThruOuterChild(pexprLogicalChild))
	{
		PushThruOuterChild(mp, pexprLogicalChild, pexprPred, ppexprResult);
	}
	else
	{
		// logical child may not pass all predicates through, we need to collect
		// unpushable predicates, if any, into a top Select node
		CExpressionArray *pdrgpexprConjuncts =
			CPredicateUtils::PdrgpexprConjuncts(mp, pexprPred);
		CExpressionArray *pdrgpexprRemaining = nullptr;
		gpos::Ref<CExpression> pexpr = nullptr;
		PushThru(mp, pexprLogicalChild, pdrgpexprConjuncts, &pexpr,
				 &pdrgpexprRemaining);
		*ppexprResult = PexprSelect(mp, pexpr, pdrgpexprRemaining);
		pdrgpexprConjuncts->Release();
	}

	pexprPred->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PexprSelect
//
//	@doc:
//		Return a Select expression, if needed, with a scalar condition made of
//		given array of conjuncts
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CNormalizer::PexprSelect(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
						 CExpressionArray *pdrgpexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pdrgpexpr);

	if (0 == pdrgpexpr->Size())
	{
		// no predicate, return given expression
		pdrgpexpr->Release();
		return pexpr;
	}

	// result expression is a select over predicates
	gpos::Ref<CExpression> pexprConjunction =
		CPredicateUtils::PexprConjunction(mp, pdrgpexpr);
	gpos::Ref<CExpression> pexprSelect =
		CUtils::PexprSafeSelect(mp, pexpr, pexprConjunction);
	if (COperator::EopLogicalSelect != pexprSelect->Pop()->Eopid())
	{
		// Select node was pruned, return created expression
		return pexprSelect;
	}

	gpos::Ref<CExpression> pexprLogicalChild = (*pexprSelect)[0];
	COperator::EOperatorId eopidChild = pexprLogicalChild->Pop()->Eopid();

	// we have a Select on top of Outer Join expression, attempt simplifying expression into InnerJoin
	gpos::Ref<CExpression> pexprSimplified = nullptr;
	if (COperator::EopLogicalLeftOuterJoin == eopidChild &&
		FSimplifySelectOnOuterJoin(mp, pexprLogicalChild, (*pexprSelect)[1],
								   &pexprSimplified))
	{
		// simplification succeeded, normalize resulting expression
		pexprSelect->Release();
		gpos::Ref<CExpression> pexprResult =
			PexprNormalize(mp, pexprSimplified);
		pexprSimplified->Release();

		return pexprResult;
	}
	else if (COperator::EopLogicalFullOuterJoin == eopidChild &&
			 FSimplifySelectOnFullJoin(mp, pexprLogicalChild, (*pexprSelect)[1],
									   &pexprSimplified))
	{
		// simplification succeeded, normalize resulting expression
		pexprSelect->Release();
		gpos::Ref<CExpression> pexprResult =
			PexprNormalize(mp, pexprSimplified);
		pexprSimplified->Release();

		return pexprResult;
	}

	// simplification failed, return created Select expression
	return pexprSelect;
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThruUnaryWithoutScalarChild
//
//	@doc:
//		Push a conjunct through unary operator without scalar child
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThruUnaryWithoutScalarChild(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
	gpos::Ref<CExpression> pexprConj, gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != pexprLogical);
	GPOS_ASSERT(1 == pexprLogical->Arity());
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	// break scalar expression to conjuncts
	CExpressionArray *pdrgpexprConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprConj);

	// get logical child
	gpos::Ref<CExpression> pexprLogicalChild = (*pexprLogical)[0];

	// push conjuncts through the logical child
	gpos::Ref<CExpression> pexprNewLogicalChild = nullptr;
	CExpressionArray *pdrgpexprUnpushable = nullptr;
	PushThru(mp, pexprLogicalChild, pdrgpexprConjuncts, &pexprNewLogicalChild,
			 &pdrgpexprUnpushable);
	pdrgpexprConjuncts->Release();

	// create a new logical expression based on recursion results
	gpos::Ref<COperator> pop = pexprLogical->Pop();
	pop->AddRef();
	gpos::Ref<CExpression> pexprNewLogical =
		GPOS_NEW(mp) CExpression(mp, pop, pexprNewLogicalChild);
	*ppexprResult = PexprSelect(mp, pexprNewLogical, pdrgpexprUnpushable);
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThruUnaryWithScalarChild
//
//	@doc:
//		Push a conjunct through a unary operator with scalar child
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThruUnaryWithScalarChild(CMemoryPool *mp,
										  gpos::Ref<CExpression> pexprLogical,
										  gpos::Ref<CExpression> pexprConj,
										  gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != pexprLogical);
	GPOS_ASSERT(2 == pexprLogical->Arity());
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	// get logical and scalar children
	gpos::Ref<CExpression> pexprLogicalChild = (*pexprLogical)[0];
	gpos::Ref<CExpression> pexprScalarChild = (*pexprLogical)[1];

	// push conjuncts through the logical child
	gpos::Ref<CExpression> pexprNewLogicalChild = nullptr;
	CExpressionArray *pdrgpexprUnpushable = nullptr;

	// break scalar expression to conjuncts
	CExpressionArray *pdrgpexprConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprConj);

	PushThru(mp, pexprLogicalChild, pdrgpexprConjuncts, &pexprNewLogicalChild,
			 &pdrgpexprUnpushable);
	pdrgpexprConjuncts->Release();

	// create a new logical expression based on recursion results
	gpos::Ref<COperator> pop = pexprLogical->Pop();
	pop->AddRef();
	pexprScalarChild->AddRef();
	gpos::Ref<CExpression> pexprNewLogical = GPOS_NEW(mp)
		CExpression(mp, pop, pexprNewLogicalChild, pexprScalarChild);
	*ppexprResult = PexprSelect(mp, pexprNewLogical, pdrgpexprUnpushable);
}

//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::SplitConjunctForSeqPrj
//
//	@doc:
//		Split the given conjunct into pushable and unpushable predicates
//		for a sequence project expression
//
//---------------------------------------------------------------------------
void
CNormalizer::SplitConjunctForSeqPrj(CMemoryPool *mp,
									gpos::Ref<CExpression> pexprSeqPrj,
									gpos::Ref<CExpression> pexprConj,
									CExpressionArray **ppdrgpexprPushable,
									CExpressionArray **ppdrgpexprUnpushable)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppdrgpexprPushable);
	GPOS_ASSERT(nullptr != ppdrgpexprUnpushable);

	*ppdrgpexprPushable = GPOS_NEW(mp) CExpressionArray(mp);
	*ppdrgpexprUnpushable = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionArray *pdrgpexprPreds =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprConj);
	const ULONG ulPreds = pdrgpexprPreds->Size();
	for (ULONG ul = 0; ul < ulPreds; ul++)
	{
		gpos::Ref<CExpression> pexprPred = (*pdrgpexprPreds)[ul];
		pexprPred->AddRef();
		if (FPushableThruSeqPrjChild(pexprSeqPrj, pexprPred))
		{
			(*ppdrgpexprPushable)->Append(pexprPred);
		}
		else
		{
			(*ppdrgpexprUnpushable)->Append(pexprPred);
		}
	}
	pdrgpexprPreds->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThruSeqPrj
//
//	@doc:
//		Push a conjunct through a sequence project expression
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThruSeqPrj(CMemoryPool *mp, gpos::Ref<CExpression> pexprSeqPrj,
							gpos::Ref<CExpression> pexprConj,
							gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);
	GPOS_ASSERT(CLogical::EopLogicalSequenceProject ==
				pexprSeqPrj->Pop()->Eopid());
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	// get logical and scalar children
	gpos::Ref<CExpression> pexprLogicalChild = (*pexprSeqPrj)[0];
	gpos::Ref<CExpression> pexprScalarChild = (*pexprSeqPrj)[1];

	// break scalar expression to pushable and unpushable conjuncts
	CExpressionArray *pdrgpexprPushable = nullptr;
	CExpressionArray *pdrgpexprUnpushable = nullptr;
	SplitConjunctForSeqPrj(mp, pexprSeqPrj, pexprConj, &pdrgpexprPushable,
						   &pdrgpexprUnpushable);

	gpos::Ref<CExpression> pexprNewLogicalChild = nullptr;
	if (0 < pdrgpexprPushable->Size())
	{
		gpos::Ref<CExpression> pexprPushableConj =
			CPredicateUtils::PexprConjunction(mp, pdrgpexprPushable);
		PushThru(mp, pexprLogicalChild, pexprPushableConj,
				 &pexprNewLogicalChild);
		pexprPushableConj->Release();
	}
	else
	{
		// no pushable predicates on top of sequence project,
		// we still need to process child recursively to push-down child's own predicates
		pdrgpexprPushable->Release();
		pexprNewLogicalChild = PexprNormalize(mp, pexprLogicalChild);
	}

	// create a new logical expression based on recursion results
	gpos::Ref<COperator> pop = pexprSeqPrj->Pop();
	pop->AddRef();
	pexprScalarChild->AddRef();
	gpos::Ref<CExpression> pexprNewLogical = GPOS_NEW(mp)
		CExpression(mp, pop, pexprNewLogicalChild, pexprScalarChild);

	// create a select node for remaining predicates, if any
	*ppexprResult = PexprSelect(mp, pexprNewLogical, pdrgpexprUnpushable);
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThruSetOp
//
//	@doc:
//		Push a conjunct through set operation
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThruSetOp(CMemoryPool *mp, gpos::Ref<CExpression> pexprSetOp,
						   gpos::Ref<CExpression> pexprConj,
						   gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != pexprSetOp);
	GPOS_ASSERT(CUtils::FLogicalSetOp(pexprSetOp->Pop()));
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	gpos::Ref<CLogicalSetOp> popSetOp =
		CLogicalSetOp::PopConvert(pexprSetOp->Pop());
	CColRefArray *pdrgpcrOutput = popSetOp->PdrgpcrOutput();
	gpos::Ref<CColRefSet> pcrsOutput =
		GPOS_NEW(mp) CColRefSet(mp, pdrgpcrOutput);
	CColRef2dArray *pdrgpdrgpcrInput = popSetOp->PdrgpdrgpcrInput();
	CExpressionArray *pdrgpexprNewChildren = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG arity = pexprSetOp->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexprSetOp)[ul];
		CColRefArray *pdrgpcrChild = (*pdrgpdrgpcrInput)[ul];
		gpos::Ref<CColRefSet> pcrsChild =
			GPOS_NEW(mp) CColRefSet(mp, pdrgpcrChild);

		pexprConj->AddRef();
		gpos::Ref<CExpression> pexprRemappedConj = pexprConj;
		if (!pcrsChild->Equals(pcrsOutput))
		{
			// child columns are different from SetOp output columns,
			// we need to fix conjunct by mapping output columns to child columns,
			// columns that are not in the output of SetOp child need also to be re-mapped
			// to new columns,
			//
			// for example, if the conjunct looks like 'x > (select max(y) from T)'
			// and the SetOp child produces only column x, we need to create a new
			// conjunct that looks like 'x1 > (select max(y1) from T)'
			// where x1 is a copy of x, and y1 is a copy of y
			//
			// this is achieved by passing (must_exist = True) flag below, which enforces
			// creating column copies for columns not already in the given map
			UlongToColRefMap *colref_mapping =
				CUtils::PhmulcrMapping(mp, pdrgpcrOutput, pdrgpcrChild);
			pexprRemappedConj->Release();
			pexprRemappedConj = pexprConj->PexprCopyWithRemappedColumns(
				mp, colref_mapping, true /*must_exist*/);
			colref_mapping->Release();
		}

		gpos::Ref<CExpression> pexprNewChild = nullptr;
		PushThru(mp, pexprChild, pexprRemappedConj, &pexprNewChild);
		pdrgpexprNewChildren->Append(pexprNewChild);

		pexprRemappedConj->Release();
		pcrsChild->Release();
	}

	pcrsOutput->Release();
	popSetOp->AddRef();
	*ppexprResult =
		GPOS_NEW(mp) CExpression(mp, popSetOp, pdrgpexprNewChildren);
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThruJoin
//
//	@doc:
//		Push a conjunct through a join
//
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThruJoin(CMemoryPool *mp, gpos::Ref<CExpression> pexprJoin,
						  gpos::Ref<CExpression> pexprConj,
						  gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	gpos::Ref<COperator> pop = pexprJoin->Pop();
	gpos::Ref<CLogicalNAryJoin> popNAryJoin = CLogicalNAryJoin::PopConvert(pop);
	const ULONG arity = pexprJoin->Arity();
	BOOL fLASApply = CUtils::FLeftAntiSemiApply(pop);
	COperator::EOperatorId op_id = pop->Eopid();
	BOOL fOuterJoin = COperator::EopLogicalLeftOuterJoin == op_id ||
					  COperator::EopLogicalLeftOuterApply == op_id ||
					  COperator::EopLogicalLeftOuterCorrelatedApply == op_id;
	BOOL fMixedInnerOuterJoin =
		(popNAryJoin && popNAryJoin->HasOuterJoinChildren());

	if (fOuterJoin && !CUtils::FScalarConstTrue(pexprConj))
	{
		// whenever possible, push incoming predicate through outer join's outer child,
		// recursion will eventually reach the rest of PushThruJoin() to process join predicates
		PushThruOuterChild(mp, pexprJoin, pexprConj, ppexprResult);

		return;
	}

	// if we have an nary join with only inner joins and a false scalar condition,
	// simplify the expression to a constant false. Trying to normalize this would
	// improperly cause the scalar condition to be pulled into one of the predicates,
	// and leave the condition as a const false
	if (popNAryJoin && !fMixedInnerOuterJoin &&
		CUtils::FScalarConstFalse(pexprConj))
	{
		gpos::Ref<COperator> popCTG = GPOS_NEW(mp) CLogicalConstTableGet(
			mp, pexprJoin->DeriveOutputColumns()->Pdrgpcr(mp),
			GPOS_NEW(mp) IDatum2dArray(mp));
		*ppexprResult = GPOS_NEW(mp) CExpression(mp, popCTG);

		return;
	}

	// combine conjunct with join predicate
	gpos::Ref<CExpression> pexprScalar = (*pexprJoin)[arity - 1];
	if (fMixedInnerOuterJoin)
	{
		GPOS_ASSERT(COperator::EopScalarNAryJoinPredList ==
					pexprScalar->Pop()->Eopid());
		pexprScalar = (*pexprScalar)[0];

		if (COperator::EopScalarNAryJoinPredList == pexprConj->Pop()->Eopid())
		{
			pexprConj = (*pexprConj)[0];
		}
	}
	gpos::Ref<CExpression> pexprPred =
		CPredicateUtils::PexprConjunction(mp, pexprScalar, pexprConj);

	// break predicate to conjuncts
	CExpressionArray *pdrgpexprConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprPred);
	pexprPred->Release();

	// push predicates through children and compute new child expressions
	CExpressionArray *pdrgpexprChildren = GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < arity - 1; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexprJoin)[ul];
		gpos::Ref<CExpression> pexprNewChild = nullptr;
		if (fLASApply)
		{
			// do not push anti-semi-apply predicates to any of the children
			pexprNewChild = PexprNormalize(mp, pexprChild);
			pdrgpexprChildren->Append(pexprNewChild);
			continue;
		}

		if (0 == ul && fOuterJoin)
		{
			// do not push outer join predicates through outer child
			// otherwise, we will throw away outer child's tuples that should
			// be part of the join result
			pexprNewChild = PexprNormalize(mp, pexprChild);
			pdrgpexprChildren->Append(pexprNewChild);
			continue;
		}

		if (fMixedInnerOuterJoin && !popNAryJoin->IsInnerJoinChild(ul))
		{
			// this is similar to what PushThruOuterChild does, only push the
			// preds to those children that are using an inner join
			pexprNewChild = PexprNormalize(mp, pexprChild);
			pdrgpexprChildren->Append(pexprNewChild);
			continue;
		}

		CExpressionArray *pdrgpexprRemaining = nullptr;
		PushThru(mp, pexprChild, pdrgpexprConjuncts, &pexprNewChild,
				 &pdrgpexprRemaining);
		pdrgpexprChildren->Append(pexprNewChild);

		pdrgpexprConjuncts->Release();
		pdrgpexprConjuncts = pdrgpexprRemaining;
	}

	// remaining conjuncts become the new join predicate
	gpos::Ref<CExpression> pexprNewScalar =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConjuncts);
	if (fMixedInnerOuterJoin)
	{
		CExpressionArray *naryJoinPredicates =
			GPOS_NEW(mp) CExpressionArray(mp);
		naryJoinPredicates->Append(pexprNewScalar);
		gpos::Ref<CExpression> pexprScalar = (*pexprJoin)[arity - 1];
		ULONG scalar_arity = pexprScalar->Arity();

		for (ULONG count = 1; count < scalar_arity; count++)
		{
			gpos::Ref<CExpression> pexprChild = (*pexprScalar)[count];
			pexprChild->AddRef();
			naryJoinPredicates->Append(pexprChild);
		}

		gpos::Ref<CExpression> nAryJoinPredicateList = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarNAryJoinPredList(mp), naryJoinPredicates);
		pdrgpexprChildren->Append(nAryJoinPredicateList);
	}
	else
	{
		pdrgpexprChildren->Append(pexprNewScalar);
	}

	// create a new join expression
	pop->AddRef();
	gpos::Ref<CExpression> pexprJoinWithInferredPred =
		GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
	gpos::Ref<CExpression> pexprJoinWithoutInferredPred = nullptr;

	// remove inferred predicate from the join expression. inferred predicate can impact the cost
	// of the join node as the node will have to project more columns even though they are not
	// used by the above nodes. So, better to remove them from the join after all the inferred predicates
	// are pushed down.
	// We don't do this for CTE as removing inferred predicates from CTE with inlining enabled may
	// cause the relational properties of two group to be same and can result in group merges,
	// which can lead to circular derivations, we should fix the bug to avoid circular references
	// before we enable it for Inlined CTEs.
	if (CUtils::CanRemoveInferredPredicates(pop->Eopid()) &&
		!COptCtxt::PoctxtFromTLS()->Pcteinfo()->FEnableInlining())
	{
		// Subqueries should be un-nested first so that we can infer any predicates if possible,
		// if they are not un-nested, they don't have any inferred predicates to remove.
		// ORCA only infers predicates for subqueries after they are un-nested.
		BOOL has_subquery =
			CUtils::FHasSubqueryOrApply(pexprJoinWithInferredPred);
		if (!has_subquery)
		{
			pexprJoinWithoutInferredPred = CUtils::MakeJoinWithoutInferredPreds(
				mp, pexprJoinWithInferredPred);
			pexprJoinWithInferredPred->Release();
			*ppexprResult = pexprJoinWithoutInferredPred;
			return;
		}
	}
	*ppexprResult = pexprJoinWithInferredPred;
}

//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::FChild
//
//	@doc:
//		Return true if second expression is a child of first expression
//
//---------------------------------------------------------------------------
BOOL
CNormalizer::FChild(gpos::Ref<CExpression> pexpr,
					gpos::Ref<CExpression> pexprChild)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pexprChild);

	BOOL fFound = false;
	const ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; !fFound && ul < arity; ul++)
	{
		fFound = ((*pexpr)[ul] == pexprChild);
	}

	return fFound;
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThru
//
//	@doc:
//		Hub for pushing a conjunct through a logical expression
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThru(CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
					  gpos::Ref<CExpression> pexprConj,
					  gpos::Ref<CExpression> *ppexprResult)
{
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != pexprLogical);
	GPOS_ASSERT(nullptr != pexprConj);
	GPOS_ASSERT(nullptr != ppexprResult);

	// TODO: 01/13/2012 - ; predicate push down with set returning functions

	if (0 == pexprLogical->Arity())
	{
		// end recursion early for leaf patterns extracted from memo
		pexprLogical->AddRef();
		pexprConj->AddRef();
		*ppexprResult = CUtils::PexprSafeSelect(mp, pexprLogical, pexprConj);
		return;
	}

	// find the push thru function corresponding to the given operator
	switch (pexprLogical->Pop()->Eopid())
	{
		case COperator::EopLogicalSelect:
			PushThruSelect(mp, pexprLogical, pexprConj, ppexprResult);
			break;

		case COperator::EopLogicalProject:
		case COperator::EopLogicalGbAgg:
			PushThruUnaryWithScalarChild(mp, pexprLogical, pexprConj,
										 ppexprResult);
			break;

		case COperator::EopLogicalSequenceProject:
			PushThruSeqPrj(mp, pexprLogical, pexprConj, ppexprResult);
			break;

		case COperator::EopLogicalCTEAnchor:
			PushThruUnaryWithoutScalarChild(mp, pexprLogical, pexprConj,
											ppexprResult);
			break;

		case COperator::EopLogicalUnion:
		case COperator::EopLogicalUnionAll:
		case COperator::EopLogicalIntersect:
		case COperator::EopLogicalIntersectAll:
		case COperator::EopLogicalDifference:
		case COperator::EopLogicalDifferenceAll:
			PushThruSetOp(mp, pexprLogical, pexprConj, ppexprResult);
			break;

		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalNAryJoin:
		case COperator::EopLogicalInnerApply:
		case COperator::EopLogicalInnerCorrelatedApply:
		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalLeftOuterApply:
		case COperator::EopLogicalLeftSemiApply:
		case COperator::EopLogicalLeftSemiApplyIn:
		case COperator::EopLogicalLeftSemiCorrelatedApplyIn:
		case COperator::EopLogicalLeftAntiSemiApply:
		case COperator::EopLogicalLeftAntiSemiApplyNotIn:
		case COperator::EopLogicalLeftAntiSemiCorrelatedApplyNotIn:
		case COperator::EopLogicalLeftSemiJoin:
			PushThruJoin(mp, pexprLogical, pexprConj, ppexprResult);
			break;
		case COperator::EopLogicalLeftOuterCorrelatedApply:
		default:
		{
			if (COperator::EopLogicalLeftOuterCorrelatedApply ==
					pexprLogical->Pop()->Eopid() &&
				CLogicalLeftOuterCorrelatedApply::PopConvert(
					pexprLogical->Pop())
					->IsPredicatePushDownAllowed())
			{
				PushThruJoin(mp, pexprLogical, pexprConj, ppexprResult);
				break;
			}

			// can't push predicates through, start a new normalization path
			gpos::Ref<CExpression> pexprNormalized =
				PexprRecursiveNormalize(mp, pexprLogical);
			*ppexprResult = pexprNormalized;
			if (!FChild(pexprLogical, pexprConj))
			{
				// add select node on top of the result for the given predicate
				pexprConj->AddRef();
				*ppexprResult =
					CUtils::PexprSafeSelect(mp, pexprNormalized, pexprConj);
			}
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PushThru
//
//	@doc:
//		Push an array of conjuncts through a logical expression;
//		compute an array of unpushable conjuncts
//
//---------------------------------------------------------------------------
void
CNormalizer::PushThru(CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
					  CExpressionArray *pdrgpexprConjuncts,
					  gpos::Ref<CExpression> *ppexprResult,
					  CExpressionArray **ppdrgpexprRemaining)
{
	GPOS_ASSERT(nullptr != pexprLogical);
	GPOS_ASSERT(nullptr != pdrgpexprConjuncts);
	GPOS_ASSERT(nullptr != ppexprResult);

	CExpressionArray *pdrgpexprPushable = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionArray *pdrgpexprUnpushable = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG size = pdrgpexprConjuncts->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CExpression> pexprConj = (*pdrgpexprConjuncts)[ul];
		pexprConj->AddRef();

		if (FPushable(pexprLogical, pexprConj))
		{
			pdrgpexprPushable->Append(pexprConj);
		}
		else
		{
			pdrgpexprUnpushable->Append(pexprConj);
		}
	}

	// push through a conjunction of all pushable predicates
	gpos::Ref<CExpression> pexprPred =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprPushable);
	if (FPushThruOuterChild(pexprLogical))
	{
		PushThruOuterChild(mp, pexprLogical, pexprPred, ppexprResult);
	}
	else
	{
		PushThru(mp, pexprLogical, pexprPred, ppexprResult);
	}
	pexprPred->Release();

	*ppdrgpexprRemaining = pdrgpexprUnpushable;
}


//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PexprNormalize
//
//	@doc:
//		Main driver
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CNormalizer::PexprNormalize(CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != pexpr);

	if (0 == pexpr->Arity())
	{
		// end recursion early for leaf patterns extracted from memo
		pexpr->AddRef();
		return pexpr;
	}

	gpos::Ref<CExpression> pexprResult = nullptr;
	gpos::Ref<COperator> pop = pexpr->Pop();
	if (pop->FLogical() && CLogical::PopConvert(pop)->FSelectionOp())
	{
		if (FPushThruOuterChild(pexpr))
		{
			gpos::Ref<CExpression> pexprConstTrue =
				CUtils::PexprScalarConstBool(mp, true /*value*/);
			PushThru(mp, pexpr, pexprConstTrue, &pexprResult);
			pexprConstTrue->Release();
		}
		else
		{
			// add-ref all children except scalar predicate
			const ULONG arity = pexpr->Arity();
			CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
			for (ULONG ul = 0; ul < arity - 1; ul++)
			{
				gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
				pexprChild->AddRef();
				pdrgpexpr->Append(pexprChild);
			}

			// normalize scalar predicate and construct a new expression
			gpos::Ref<CExpression> pexprPred = (*pexpr)[pexpr->Arity() - 1];
			gpos::Ref<CExpression> pexprPredNormalized =
				PexprRecursiveNormalize(mp, pexprPred);
			pdrgpexpr->Append(pexprPredNormalized);
			gpos::Ref<COperator> pop = pexpr->Pop();
			pop->AddRef();
			gpos::Ref<CExpression> pexprNew =
				GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);

			// push normalized predicate through
			PushThru(mp, pexprNew, pexprPredNormalized, &pexprResult);
			pexprNew->Release();
		}
	}
	else
	{
		pexprResult = PexprRecursiveNormalize(mp, pexpr);
	}
	GPOS_ASSERT(nullptr != pexprResult);

	return pexprResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PexprPullUpAndCombineProjects
//
//	@doc:
//		Pulls up logical projects as far as possible, and combines consecutive
//		projects if possible
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CNormalizer::PexprPullUpAndCombineProjects(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	BOOL *pfSuccess	 // output to indicate whether anything was pulled up
)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pfSuccess);

	gpos::Ref<COperator> pop = pexpr->Pop();
	const ULONG arity = pexpr->Arity();
	if (!pop->FLogical() || 0 == arity)
	{
		pexpr->AddRef();
		return pexpr;
	}

	CExpressionArray *pdrgpexprChildren = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionArray *pdrgpexprPrElPullUp = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexpr);

	gpos::Ref<CColRefSet> pcrsOutput = pexpr->DeriveOutputColumns();

	// extract the columns used by the scalar expression and the operator itself (for grouping, sorting, etc.)
	gpos::Ref<CColRefSet> pcrsUsed = exprhdl.PcrsUsedColumns(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprPullUpAndCombineProjects(mp, (*pexpr)[ul], pfSuccess);
		if (pop->FLogical() &&
			CLogical::PopConvert(pop)->FCanPullProjectionsUp(ul) &&
			COperator::EopLogicalProject == pexprChild->Pop()->Eopid())
		{
			// this child is a project - see if any project elements can be pulled up
			gpos::Ref<CExpression> pexprNewChild = PexprPullUpProjectElements(
				mp, pexprChild, pcrsUsed, pcrsOutput, pdrgpexprPrElPullUp);

			pexprChild->Release();
			pexprChild = pexprNewChild;
		}

		pdrgpexprChildren->Append(pexprChild);
	}

	pcrsUsed->Release();
	pop->AddRef();

	if (0 < pdrgpexprPrElPullUp->Size() &&
		COperator::EopLogicalProject == pop->Eopid())
	{
		// some project elements have been pulled up and the original expression
		// was a project - combine its project list with the pulled up project elements
		GPOS_ASSERT(2 == pdrgpexprChildren->Size());
		*pfSuccess = true;
		gpos::Ref<CExpression> pexprRelational = (*pdrgpexprChildren)[0];
		gpos::Ref<CExpression> pexprPrLOld = (*pdrgpexprChildren)[1];
		pexprRelational->AddRef();

		CUtils::AddRefAppend(pdrgpexprPrElPullUp, pexprPrLOld->PdrgPexpr());
		pdrgpexprChildren->Release();
		gpos::Ref<CExpression> pexprPrjList = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprPrElPullUp);

#ifdef GPOS_DEBUG
		gpos::Ref<CColRefSet> availableCRs = GPOS_NEW(mp) CColRefSet(mp);

		availableCRs->Include(pexprRelational->DeriveOutputColumns());
		availableCRs->Include(pexpr->DeriveOuterReferences());
		// check that the new project node has all the values it needs
		GPOS_ASSERT(
			availableCRs->ContainsAll(pexprPrjList->DeriveUsedColumns()));
		availableCRs->Release();
#endif

		return GPOS_NEW(mp) CExpression(mp, pop, pexprRelational, pexprPrjList);
	}

	gpos::Ref<CExpression> pexprOutput =
		GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);

	if (0 == pdrgpexprPrElPullUp->Size())
	{
		// no project elements were pulled up
		pdrgpexprPrElPullUp->Release();
		return pexprOutput;
	}

	// some project elements were pulled - add a project on top of output expression
	*pfSuccess = true;
	gpos::Ref<CExpression> pexprPrjList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprPrElPullUp);
	GPOS_ASSERT(pexprOutput->DeriveOutputColumns()->ContainsAll(
		pexprPrjList->DeriveUsedColumns()));

	return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp),
									pexprOutput, pexprPrjList);
}

//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PexprPullUpProjectElements
//
//	@doc:
//		Pull up project elements from the given projection expression that do not
//		exist in the given used columns set. The pulled up project elements must only
//		use columns that are in the output columns of the parent operator. Returns
//		a new expression that does not have the pulled up project elements. These
//		project elements are appended to the given array.
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CNormalizer::PexprPullUpProjectElements(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefSet> pcrsUsed, gpos::Ref<CColRefSet> pcrsOutput,
	CExpressionArray
		*pdrgpexprPrElPullUp  // output: the pulled-up project elements
)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(COperator::EopLogicalProject == pexpr->Pop()->Eopid());
	GPOS_ASSERT(nullptr != pcrsUsed);
	GPOS_ASSERT(nullptr != pdrgpexprPrElPullUp);

	if (2 != pexpr->Arity())
	{
		// the project's children were not extracted as part of the pattern in this xform
		GPOS_ASSERT(0 == pexpr->Arity());
		pexpr->AddRef();
		return pexpr;
	}

	CExpressionArray *pdrgpexprPrElNoPullUp = GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CExpression> pexprPrL = (*pexpr)[1];

	const ULONG ulProjElements = pexprPrL->Arity();
	for (ULONG ul = 0; ul < ulProjElements; ul++)
	{
		gpos::Ref<CExpression> pexprPrEl = (*pexprPrL)[ul];
		gpos::Ref<CScalarProjectElement> popPrEl =
			CScalarProjectElement::PopConvert(pexprPrEl->Pop());
		gpos::Ref<CColRef> pcrDefined = popPrEl->Pcr();
		gpos::Ref<CColRefSet> pcrsUsedByProjElem =
			pexprPrEl->DeriveUsedColumns();

		// a proj elem can be pulled up only if the defined column is not in
		// pcrsUsed and its used columns are in pcrOutput
		// NB we don't pull up projections that call set-returning functions
		pexprPrEl->AddRef();

		if (!pcrsUsed->FMember(pcrDefined) &&
			pcrsOutput->ContainsAll(pcrsUsedByProjElem) &&
			!pexprPrEl->DeriveHasNonScalarFunction())
		{
			pdrgpexprPrElPullUp->Append(pexprPrEl);
		}
		else
		{
			pdrgpexprPrElNoPullUp->Append(pexprPrEl);
		}
	}

	gpos::Ref<CExpression> pexprNew = (*pexpr)[0];
	pexprNew->AddRef();
	if (0 == pdrgpexprPrElNoPullUp->Size())
	{
		pdrgpexprPrElNoPullUp->Release();
	}
	else
	{
		// some project elements could not be pulled up - need a project here
		gpos::Ref<CExpression> pexprPrjList = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprPrElNoPullUp);
		pexprNew = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalProject(mp), pexprNew, pexprPrjList);
	}

	return pexprNew;
}

//---------------------------------------------------------------------------
//	@function:
//		CNormalizer::PexprPullUpProjections
//
//	@doc:
//		Pulls up logical projects as far as possible, and combines consecutive
//		projects if possible
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CNormalizer::PexprPullUpProjections(CMemoryPool *mp,
									gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	BOOL fPullUp = true;
	pexpr->AddRef();
	gpos::Ref<CExpression> pexprOutput = pexpr;

	while (fPullUp)
	{
		fPullUp = false;

		gpos::Ref<CExpression> pexprOutputNew =
			PexprPullUpAndCombineProjects(mp, pexprOutput, &fPullUp);
		pexprOutput->Release();
		pexprOutput = pexprOutputNew;
	}

	GPOS_ASSERT(FLocalColsSubsetOfInputCols(mp, pexprOutput));

	return pexprOutput;
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//     @function:
//             CNormalizer::FLocalColsSubsetOfInputCols
//
//     @doc:
//             Check if the columns used by the operator are a subset of its input columns
//
//---------------------------------------------------------------------------
BOOL
CNormalizer::FLocalColsSubsetOfInputCols(CMemoryPool *mp,
										 gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_CHECK_STACK_SIZE;

	CExpressionHandle exprhdl(mp);
	if (nullptr != pexpr->Pgexpr())
	{
		exprhdl.Attach(pexpr->Pgexpr());
	}
	else
	{
		exprhdl.Attach(pexpr);
	}
	exprhdl.DeriveProps(nullptr /*pdpctxt*/);

	BOOL fValid = true;
	if (pexpr->Pop()->FLogical())
	{
		if (0 == exprhdl.UlNonScalarChildren())
		{
			return true;
		}

		gpos::Ref<CColRefSet> pcrsInput = GPOS_NEW(mp) CColRefSet(mp);

		const ULONG arity = exprhdl.Arity();
		for (ULONG ul = 0; ul < arity; ul++)
		{
			if (!exprhdl.FScalarChild(ul))
			{
				pcrsInput->Include(exprhdl.DeriveOutputColumns(ul));
			}
		}

		// check if the operator's locally used columns are a subset of the input columns
		gpos::Ref<CColRefSet> pcrsUsedOp = exprhdl.PcrsUsedColumns(mp);
		pcrsUsedOp->Exclude(exprhdl.DeriveOuterReferences());

		fValid = pcrsInput->ContainsAll(pcrsUsedOp);

		// release
		pcrsInput->Release();
		pcrsUsedOp->Release();
	}

	// check if its children are valid
	const ULONG ulExprArity = pexpr->Arity();
	for (ULONG ulChildIdx = 0; ulChildIdx < ulExprArity && fValid; ulChildIdx++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ulChildIdx];
		fValid = FLocalColsSubsetOfInputCols(mp, pexprChild);
	}

	return fValid;
}

#endif	//GPOS_DEBUG

// EOF
