//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2012 EMC Corp.
//
//	@filename:
//		CExpressionPreprocessor.cpp
//
//	@doc:
//		Expression tree preprocessing routines, needed to prepare an input
//		logical expression to be optimized
//---------------------------------------------------------------------------

#include "gpopt/operators/CExpressionPreprocessor.h"

#include "gpos/base.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/common/CAutoTimer.h"

#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CDrvdPropCtxt.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/exception.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/operators/CExpressionFactorizer.h"
#include "gpopt/operators/CExpressionUtils.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalCTEProducer.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalDynamicGet.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CLogicalUpdate.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/COrderedAggPreprocessor.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarNAryJoinPredList.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"
#include "gpopt/operators/CWindowPreprocessor.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/translate/CTranslatorDXLToExpr.h"
#include "gpopt/xforms/CXform.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDType.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

static void UpdateExprToConstantPredicateMapping(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprFilter,
	gpos::Ref<ExprToConstantMap> phmExprToConst, BOOL doInsert);

static gpos::Ref<CExpression> SubstituteConstantIdentifier(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<ExprToConstantMap> phmExprToConst);

// maximum number of equality predicates to be derived from existing equalities
#define GPOPT_MAX_DERIVED_PREDS 50

// eliminate self comparisons in the given expression
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprEliminateSelfComparison(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	if (CUtils::FScalarCmp(pexpr))
	{
		return CPredicateUtils::PexprEliminateSelfComparison(mp, pexpr);
	}

	// recursively process children
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprEliminateSelfComparison(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	gpos::Ref<COperator> pop = pexpr->Pop();


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// remove superfluous equality operations
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprPruneSuperfluousEquality(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	if (pexpr->Pop()->FScalar())
	{
		return CPredicateUtils::PexprPruneSuperfluosEquality(mp, pexpr);
	}

	// recursively process children
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprPruneSuperfluousEquality(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	gpos::Ref<COperator> pop = pexpr->Pop();

	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// an existential subquery whose inner expression is a GbAgg
// with no grouping columns is replaced with a Boolean constant
//
//		Example:
//
//			exists(select sum(i) from X) --> True
//			not exists(select sum(i) from X) --> False
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprTrimExistentialSubqueries(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (CUtils::FExistentialSubquery(pop))
	{
		gpos::Ref<CExpression> pexprInner = (*pexpr)[0];
		if (COperator::EopLogicalGbAgg == pexprInner->Pop()->Eopid() &&
			0 ==
				CLogicalGbAgg::PopConvert(pexprInner->Pop())->Pdrgpcr()->Size())
		{
			GPOS_ASSERT(0 < (*pexprInner)[1]->Arity() &&
						"Project list of GbAgg is expected to be non-empty");
			BOOL fValue = true;
			if (COperator::EopScalarSubqueryNotExists == pop->Eopid())
			{
				fValue = false;
			}
			return CUtils::PexprScalarConstBool(mp, fValue);
		}
	}

	// recursively process children
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprTrimExistentialSubqueries(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	if (CPredicateUtils::FAnd(pexpr))
	{
		return CPredicateUtils::PexprConjunction(mp, pdrgpexprChildren);
	}

	if (CPredicateUtils::FOr(pexpr))
	{
		return CPredicateUtils::PexprDisjunction(mp, pdrgpexprChildren);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}



// a quantified subquery with maxcard 1 is simplified as a scalar subquery
//
// Example:
//		a = ANY (select sum(i) from X) --> a = (select sum(i) from X)
//		a <> ALL (select sum(i) from X) --> a <> (select sum(i) from X)
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprSimplifyQuantifiedSubqueries(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (CUtils::FQuantifiedSubquery(pop) &&
		1 == (*pexpr)[0]->DeriveMaxCard().Ull())
	{
		gpos::Ref<CExpression> pexprInner = (*pexpr)[0];

		// skip intermediate unary nodes
		gpos::Ref<CExpression> pexprChild = pexprInner;
		gpos::Ref<COperator> popChild = pexprChild->Pop();
		while (nullptr != pexprChild && CUtils::FLogicalUnary(popChild))
		{
			pexprChild = (*pexprChild)[0];
			popChild = pexprChild->Pop();
		}

		// inspect next node
		BOOL fGbAggWithoutGrpCols =
			COperator::EopLogicalGbAgg == popChild->Eopid() &&
			0 == CLogicalGbAgg::PopConvert(popChild)->Pdrgpcr()->Size();

		BOOL fOneRowConstTable =
			COperator::EopLogicalConstTableGet == popChild->Eopid() &&
			1 == CLogicalConstTableGet::PopConvert(popChild)
					 ->Pdrgpdrgpdatum()
					 ->Size();

		if (fGbAggWithoutGrpCols || fOneRowConstTable)
		{
			// quantified subquery with max card 1
			gpos::Ref<CExpression> pexprScalar = (*pexpr)[1];
			gpos::Ref<CScalarSubqueryQuantified> popSubqQuantified =
				CScalarSubqueryQuantified::PopConvert(pexpr->Pop());
			const gpos::Ref<CColRef> colref = popSubqQuantified->Pcr();

			gpos::Ref<CExpression> pexprSubquery = GPOS_NEW(mp) CExpression(
				mp,
				GPOS_NEW(mp)
					CScalarSubquery(mp, colref, false /*fGeneratedByExist*/,
									true /*fGeneratedByQuantified*/),
				pexprInner);

			CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
			gpos::Ref<IMDId> mdid = popSubqQuantified->MdIdOp();
			const CWStringConst *str =
				md_accessor->RetrieveScOp(mdid)->Mdname().GetMDName();



			return CUtils::PexprScalarCmp(mp, pexprScalar, pexprSubquery, *str,
										  mdid);
		}
	}

	// recursively process children
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprSimplifyQuantifiedSubqueries(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// preliminary unnesting of scalar subqueries
// Example:
// 		Input:   SELECT k, (SELECT (SELECT Y.i FROM Y WHERE Y.j=X.j)) from X
//		Output:  SELECT k, (SELECT Y.i FROM Y WHERE Y.j=X.j) from X
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprUnnestScalarSubqueries(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	// look for a Project Element with a scalar subquery below it
	if (CUtils::FProjElemWithScalarSubq(pexpr))
	{
		// recursively process scalar subquery
		gpos::Ref<CExpression> pexprSubq =
			PexprUnnestScalarSubqueries(mp, (*pexpr)[0]);

		// if the scalar subquery is replaced by the CScalarIdent in the previous
		// recursive call we simply return the CScalarIdent and stop preprocessing
		// at this stage.
		// +--CScalarProjectList
		//	  +--CScalarProjectElement "?column?" (2)
		//	     +--CScalarIdent "column1" (1)
		if (COperator::EopScalarIdent == pexprSubq->Pop()->Eopid())
		{
			return GPOS_NEW(mp) CExpression(mp, pop, pexprSubq);
		}

		// check if subquery is defined as a Project on Const Table
		gpos::Ref<CExpression> pexprSubqChild = (*pexprSubq)[0];
		if (CUtils::FProjectConstTableWithOneScalarSubq(pexprSubqChild))
		{
			gpos::Ref<CExpression> pexprConstTable = (*pexprSubqChild)[0];
			gpos::Ref<CExpression> pexprPrjList = (*pexprSubqChild)[1];
			GPOS_ASSERT(1 == pexprPrjList->Arity());

			gpos::Ref<CExpression> pexprPrjElem = (*pexprPrjList)[0];
			gpos::Ref<CExpression> pexprInnerSubq = (*pexprPrjElem)[0];
			GPOS_ASSERT(COperator::EopScalarSubquery ==
						pexprInnerSubq->Pop()->Eopid());

			// make sure that inner subquery has no outer references to Const Table
			// since Const Table will be eliminated in output expression
			gpos::Ref<CColRefSet> pcrsConstTableOutput =
				pexprConstTable->DeriveOutputColumns();
			gpos::Ref<CColRefSet> outer_refs =
				(*pexprInnerSubq)[0]->DeriveOuterReferences();
			if (0 == outer_refs->Size() ||
				outer_refs->IsDisjoint(pcrsConstTableOutput))
			{
				// recursively process inner subquery
				gpos::Ref<CExpression> pexprUnnestedSubq =
					PexprUnnestScalarSubqueries(mp, pexprInnerSubq);

				// the original subquery is processed and can be removed now


				// build the new Project Element after eliminating outer subquery

				return GPOS_NEW(mp) CExpression(mp, pop, pexprUnnestedSubq);
			}
		}

		// otherwise, return a Project Element with the processed outer subquery

		return GPOS_NEW(mp) CExpression(mp, pop, pexprSubq);
	}

	else if (CUtils::FScalarSubqWithConstTblGet(pexpr))
	{
		const gpos::Ref<CColRef> pcrSubq =
			CScalarSubquery::PopConvert(pexpr->Pop())->Pcr();
		gpos::Ref<CColRefSet> pcrsConstTableOutput =
			(*pexpr)[0]->DeriveOutputColumns();

		// if the subquery has outer ref, we do not make use of the output columns of constant table get.
		// In this scenairo, we replace the entire scalar subquery with a CScalarIdent with the outer reference.
		// Otherwise, the subquery remains unchanged.
		// Input:
		//   +--CScalarSubquery["b" (8)]
		//      +--CLogicalConstTableGet Columns: ["" (16)] Values: [(1)]
		// Output:
		//   +--CScalarIdent "b" (8)
		if (!pcrsConstTableOutput->FMember(pcrSubq))
		{
			gpos::Ref<CScalarSubquery> pScalarSubquery =
				CScalarSubquery::PopConvert(pexpr->Pop());
			return CUtils::PexprScalarIdent(mp, pScalarSubquery->Pcr());
		}
	}

	// recursively process children
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprUnnestScalarSubqueries(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// an intermediate limit is removed if it has neither row count nor offset
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprRemoveSuperfluousLimit(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	// if current operator is a logical limit with zero offset, and no specified
	// row count, skip to limit's logical child
	if (COperator::EopLogicalLimit == pop->Eopid() &&
		CUtils::FHasZeroOffset(pexpr) &&
		!CLogicalLimit::PopConvert(pop)->FHasCount())
	{
		gpos::Ref<CLogicalLimit> popLgLimit = CLogicalLimit::PopConvert(pop);
		if (!popLgLimit->IsTopLimitUnderDMLorCTAS() ||
			(popLgLimit->IsTopLimitUnderDMLorCTAS() &&
			 GPOS_FTRACE(EopttraceRemoveOrderBelowDML)))
		{
			return PexprRemoveSuperfluousLimit(mp, (*pexpr)[0]);
		}
	}

	// recursively process children
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprRemoveSuperfluousLimit(mp, (*pexpr)[ul]);

		pdrgpexprChildren->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// distinct is removed from a DQA if it has a max or min agg
// e.g. select max(distinct(a)) from tbl -> select max(a) from tbl
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprRemoveSuperfluousDistinctInDQA(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (COperator::EopLogicalGbAgg == pop->Eopid())
	{
		const gpos::Ref<CExpression> pexprProjectList = (*pexpr)[1];
		GPOS_ASSERT(COperator::EopScalarProjectList ==
					pexprProjectList->Pop()->Eopid());
		const ULONG arity = pexprProjectList->Arity();

		CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

		for (ULONG ul = 0; ul < arity; ul++)
		{
			gpos::Ref<CExpression> const pexprPrjElem = (*pexprProjectList)[ul];
			if (COperator::EopScalarAggFunc ==
				(*pexprPrjElem)[0]->Pop()->Eopid())
			{
				gpos::Ref<CScalarAggFunc> popAggFunc =
					CScalarAggFunc::PopConvert((*pexprPrjElem)[0]->Pop());
				gpos::Ref<IMDId> agg_child_mdid =
					CScalar::PopConvert((*pexprPrjElem)[0]->Pop())->MdidType();
				const gpos::Ref<IMDType> agg_child_type =
					md_accessor->RetrieveType(agg_child_mdid);

				if (popAggFunc->IsDistinct() &&
					popAggFunc->IsMinMax(agg_child_type))
				{
					popAggFunc->SetIsDistinct(false);
				}
			}
		}
	}


	// recursively process children
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprRemoveSuperfluousDistinctInDQA(mp, (*pexpr)[ul]);

		pdrgpexprChildren->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

//	Remove outer references from order spec inside limit, grouping columns
//	in GbAgg, and Partition/Order columns in window operators. Also handle
//	cases where we would end up with an empty groupby list and project list,
//	which is not supported.
//
//	Example, for the schema: t(a, b), s(i, j)
//	The query:
//			select * from t where a < all (select i from s order by j, b limit 1);
//		should be equivalent to:
//			select * from t where a < all (select i from s order by j limit 1);
//		after removing the outer reference (b) from the order by clause of the
//		subquery (all tuples in the subquery have the same value for the outer ref)
//
//		Similarly,
//			select * from t where a in (select count(i) from s group by j, b);
//		is equivalent to:
//			select * from t where a in (select count(i) from s group by j);
//
//		Similarly,
//			select * from t where a in (select row_number() over (partition by t.a order by t.b) from s);
//		is equivalent to:
//			select * from t where a in (select row_number() over () from s);
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprRemoveSuperfluousOuterRefs(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	// operator, possibly altered below if we need to change the operator
	gpos::Ref<COperator> pop = pexpr->Pop();
	// expression, possibly altered below if we need to change the children
	gpos::Ref<CExpression> newExpr = pexpr;

	COperator::EOperatorId op_id = pop->Eopid();
	BOOL fHasOuterRefs = (pop->FLogical() && CUtils::HasOuterRefs(pexpr));


	if (fHasOuterRefs)
	{
		// special handling for three operator types: Limit, GrbyAgg, Sequence
		if (COperator::EopLogicalLimit == op_id)
		{
			gpos::Ref<CColRefSet> outer_refs = pexpr->DeriveOuterReferences();

			gpos::Ref<CLogicalLimit> popLimit = CLogicalLimit::PopConvert(pop);
			gpos::Ref<COrderSpec> pos = popLimit->Pos();
			gpos::Ref<COrderSpec> posNew =
				pos->PosExcludeColumns(mp, outer_refs);


			pop = GPOS_NEW(mp) CLogicalLimit(
				mp, posNew, popLimit->FGlobal(), popLimit->FHasCount(),
				popLimit->IsTopLimitUnderDMLorCTAS());
		}
		else if (COperator::EopLogicalGbAgg == op_id)
		{
			gpos::Ref<CColRefSet> outer_refs = pexpr->DeriveOuterReferences();

			gpos::Ref<CLogicalGbAgg> popAgg = CLogicalGbAgg::PopConvert(pop);
			gpos::Ref<CColRefArray> colref_array =
				CUtils::PdrgpcrExcludeColumns(mp, popAgg->Pdrgpcr(),
											  outer_refs);

			gpos::Ref<CExpression> pExprProjList = (*pexpr)[1];

			// It's only valid to remove the outer reference if:
			// the projection list is NOT empty
			// or
			// the outer references are NOT the ONLY Group By column
			//
			// For example:
			// -- Cannot remove t.b from groupby, because this will produce an invalid plan
			// -- with both groupby list and project list empty, in this case we need to add
			// -- a project node below the GrbyAgg
			// select a from t where c in (select distinct t.b from s)
			//
			// -- remove t.b from groupby is ok, because there is at least one agg function: count()
			// select a from t where c in (select count(s.j) from s group by t.b)
			//
			// -- remove t.b from groupby is ok, because there is other groupby column s.j
			// select a from t where c in (select s.j from s group by t.b, s.j)
			//
			// -- remove t.b from groupby is ok, because outer reference is a
			// -- constant for each invocation of subquery
			// select a from t where c in (select count(s.j) from s group by s.i, t.b)
			//

			if (0 < pExprProjList->Arity() || 0 < colref_array->Size())
			{
				// remove outer refs from the groupby columns list
				gpos::Ref<CColRefArray> pdrgpcrMinimal =
					popAgg->PdrgpcrMinimal();
				if (nullptr != pdrgpcrMinimal)
				{
					pdrgpcrMinimal = CUtils::PdrgpcrExcludeColumns(
						mp, pdrgpcrMinimal, outer_refs);
				}

				gpos::Ref<CColRefArray> pdrgpcrArgDQA = popAgg->PdrgpcrArgDQA();
				if (nullptr != pdrgpcrArgDQA)
				{
				}


				pop = GPOS_NEW(mp) CLogicalGbAgg(
					mp, colref_array, pdrgpcrMinimal, popAgg->Egbaggtype(),
					popAgg->FGeneratesDuplicates(), pdrgpcrArgDQA);
			}
			else
			{
				// grouping_cols has outer references that can't be removed, because
				// that would make both pExprProjList and grouping_cols empty, which is not allowed.
				// The solution in this case is to add a project node below that will simply echo
				// the outer reference, and to use that newly produced ColRef as groupby column.
				gpos::Ref<CExpression> child = (*pexpr)[0];
				gpos::Ref<CExpressionArray> grouping_cols_arr =
					CUtils::PdrgpexprScalarIdents(mp, popAgg->Pdrgpcr());

				GPOS_ASSERT(0 < grouping_cols_arr->Size());


				// add a project node on top of our child
				gpos::Ref<CExpression> projectExpr = CUtils::PexprAddProjection(
					mp, child, grouping_cols_arr,
					false  // don't add to hash table,
						   // this is done at the end
						   // of preprocessing
				);


				// build a children array for the new GrbyAgg expression
				gpos::Ref<CExpressionArray> new_children =
					GPOS_NEW(mp) CExpressionArray(mp);
				new_children->Append(projectExpr);
				for (ULONG ul = 1; ul < pexpr->PdrgPexpr()->Size(); ul++)
				{
					new_children->Append((*pexpr->PdrgPexpr())[ul]);
				}

				// build a new CLogicalGbAgg operator, with a new grouping columns list
				gpos::Ref<CColRefArray> new_grouping_cols =
					GPOS_NEW(mp) CColRefArray(mp);
				gpos::Ref<CExpression> new_projected_cols = (*projectExpr)[1];
				for (ULONG ul = 0; ul < new_projected_cols->Arity(); ul++)
				{
					new_grouping_cols->Append(
						CUtils::PcrFromProjElem((*new_projected_cols)[ul]));
				}
				GPOS_ASSERT(nullptr == popAgg->PdrgpcrArgDQA());
				pop = GPOS_NEW(mp) CLogicalGbAgg(mp, new_grouping_cols, nullptr,
												 popAgg->Egbaggtype(),
												 popAgg->FGeneratesDuplicates(),
												 nullptr  // no DQA cols
				);
				// release the previous pop

				popAgg = nullptr;

				// finally, put it all together, our new GrbyAgg now has a project node below
				// it that will turn the outer reference into a produced ColRef that is used
				// as a groupby column

				newExpr = GPOS_NEW(mp) CExpression(mp, pop, new_children);
				// clean up
			}
		}
		else if (COperator::EopLogicalSequenceProject == op_id)
		{
			CExpressionHandle exprhdl(mp);
			exprhdl.Attach(pexpr);
			exprhdl.DeriveProps(nullptr /*pdpctxt*/);
			gpos::Ref<CLogicalSequenceProject> popSequenceProject =
				CLogicalSequenceProject::PopConvert(pop);
			if (popSequenceProject->FHasLocalReferencesTo(
					exprhdl.DeriveOuterReferences()))
			{
				gpos::Ref<COperator> popNew =
					popSequenceProject->PopRemoveLocalOuterRefs(mp, exprhdl);

				pop = popNew;
			}
		}
	}

	// recursively process children
	const ULONG arity = newExpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprRemoveSuperfluousOuterRefs(mp, (*newExpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	if (newExpr != pexpr)
	{
	}
	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// generate a ScalarBoolOp expression or simply return the only expression
// in the array if there is only one.
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprScalarBoolOpConvert2In(
	CMemoryPool *mp, CScalarBoolOp::EBoolOperator eboolop,
	gpos::Ref<CExpressionArray> pdrgpexpr)
{
	GPOS_ASSERT(nullptr != pdrgpexpr);
	GPOS_ASSERT(0 < pdrgpexpr->Size());

	if (1 == pdrgpexpr->Size())
	{
		// if there is one child, do not wrap it in a bool op
		gpos::Ref<CExpression> pexpr = (*pdrgpexpr)[0];


		return pexpr;
	}

	return GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CScalarBoolOp(mp, eboolop), pdrgpexpr);
}

// checks if the given expression is likely to be simplified by the constraints
// framework during array conversion. eboolop is the CScalarBoolOp type
// of the expression which contains the argument expression
BOOL
CExpressionPreprocessor::FConvert2InIsConvertable(
	gpos::Ref<CExpression> pexpr, CScalarBoolOp::EBoolOperator eboolopParent)
{
	bool fConvertableExpression = false;
	if (CPredicateUtils::FCompareIdentToConst(pexpr))
	{
		fConvertableExpression |=
			IMDType::EcmptEq ==
				CScalarCmp::PopConvert(pexpr->Pop())->ParseCmpType() &&
			CScalarBoolOp::EboolopOr == eboolopParent;
		fConvertableExpression |=
			IMDType::EcmptNEq ==
				CScalarCmp::PopConvert(pexpr->Pop())->ParseCmpType() &&
			CScalarBoolOp::EboolopAnd == eboolopParent;
	}
	else if (CPredicateUtils::FCompareIdentToConstArray(pexpr) ||
			 CPredicateUtils::FCompareCastIdentToConstArray(pexpr))
	{
		fConvertableExpression = true;
	}

	if (fConvertableExpression)
	{
		GPOS_ASSERT(0 < pexpr->Arity());
		gpos::Ref<CScalarIdent> pscid = nullptr;
		if (CUtils::FScalarIdent((*pexpr)[0]))
		{
			pscid = CScalarIdent::PopConvert((*pexpr)[0]->Pop());
		}
		else
		{
			GPOS_ASSERT(CScalarIdent::FCastedScId((*pexpr)[0]));
			pscid = CScalarIdent::PopConvert((*(*pexpr)[0])[0]->Pop());
		}
		if (!CUtils::FConstrainableType(pscid->MdidType()))
		{
			fConvertableExpression = false;
		}
	}

	return fConvertableExpression;
}

// converts series of AND or OR comparisons into array IN expressions. For
// example, x = 1 OR x = 2 will convert to x IN (1,2). This stage assumes
// the expression has been unnested using CExpressionUtils::PexprUnnest.
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprConvert2In(
	CMemoryPool *mp,
	gpos::Ref<CExpression> pexpr  // does not take ownership
)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (CPredicateUtils::FOr(pexpr) || CPredicateUtils::FAnd(pexpr))
	{
		// the bool op type of this node
		CScalarBoolOp::EBoolOperator eboolop =
			CScalarBoolOp::PopConvert(pop)->Eboolop();
		// derive constraints on all of the simple scalar children
		// and add them to a new AND or OR expression
		gpos::Ref<CExpressionArray> pdrgpexprCollapse =
			GPOS_NEW(mp) CExpressionArray(mp);
		gpos::Ref<CExpressionArray> pdrgpexprRemainder =
			GPOS_NEW(mp) CExpressionArray(mp);
		const ULONG arity = pexpr->Arity();
		for (ULONG ul = 0; ul < arity; ul++)
		{
			gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];

			if (FConvert2InIsConvertable(pexprChild, eboolop))
			{
				pdrgpexprCollapse->Append(pexprChild);
			}
			else
			{
				// recursively convert the remainder and add to the array
				pdrgpexprRemainder->Append(PexprConvert2In(mp, pexprChild));
			}
		}

		if (0 != pdrgpexprCollapse->Size())
		{
			// create the constraint, rederive the collapsed expression
			// add the new derived expr to remainder
			gpos::Ref<CColRefSetArray> colref_array = nullptr;

			CAutoRef<CExpression> apexprPreCollapse(
				GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprCollapse));
			gpos::Ref<CConstraint> apcnst(CConstraint::PcnstrFromScalarExpr(
				mp, apexprPreCollapse.Value(), &colref_array));

			GPOS_ASSERT(nullptr != apcnst);
			gpos::Ref<CExpression> pexprPostCollapse = apcnst->PexprScalar(mp);


			pdrgpexprRemainder->Append(pexprPostCollapse);
		}
		else
		{
		}

		GPOS_ASSERT(0 < pdrgpexprRemainder->Size());
		return PexprScalarBoolOpConvert2In(mp, eboolop, pdrgpexprRemainder);
	}

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CExpressionArray> pdrgexprChildren = pexpr->PdrgPexpr();
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		pdrgpexpr->Append(PexprConvert2In(mp, (*pdrgexprChildren)[ul]));
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// collapse cascaded inner and left outer joins into NAry-joins
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprCollapseJoins(CMemoryPool *mp,
											gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	const ULONG arity = pexpr->Arity();

	if (CPredicateUtils::FInnerOrNAryJoin(pexpr) ||
		(GPOS_FTRACE(EopttraceEnableLOJInNAryJoin) &&
		 CPredicateUtils::FLeftOuterJoin(pexpr)))
	{
		gpos::Ref<CExpressionArray> newChildNodes =
			GPOS_NEW(mp) CExpressionArray(mp);
		gpos::Ref<ULongPtrArray> lojChildPredIndexes =
			GPOS_NEW(mp) ULongPtrArray(mp);
		gpos::Ref<CExpressionArray> innerJoinPredicates =
			GPOS_NEW(mp) CExpressionArray(mp);
		gpos::Ref<CExpressionArray> lojPredicates =
			GPOS_NEW(mp) CExpressionArray(mp);

		CollectJoinChildrenRecursively(mp, pexpr, newChildNodes,
									   lojChildPredIndexes, innerJoinPredicates,
									   lojPredicates);

		if (lojPredicates->Size() > 0)
		{
			// each logical child must have an associated predicate index
			GPOS_ASSERT(newChildNodes->Size() == lojChildPredIndexes->Size());

			// this NAry join involves LOJs; create a CScalarNAryJoinPredList to hold
			// the information which predicates are inner join preds and which ON predicates
			// are associated with the LOJs' right children
			gpos::Ref<CExpressionArray> naryJoinPredicates =
				GPOS_NEW(mp) CExpressionArray(mp);

			// create a new CScalarNAryJoinPredList as the last child of the NAry join
			// the first child are all the inner join predicates
			naryJoinPredicates->Append(
				CPredicateUtils::PexprConjunction(mp, innerJoinPredicates));
			// the remaining children are the LOJ predicates, one by one
			for (ULONG ul = 0; ul < lojPredicates->Size(); ul++)
			{
				gpos::Ref<CExpression> predicate = (*lojPredicates)[ul];

				naryJoinPredicates->Append(predicate);
			}

			gpos::Ref<CExpression> nAryJoinPredicateList = GPOS_NEW(mp)
				CExpression(mp, GPOS_NEW(mp) CScalarNAryJoinPredList(mp),
							naryJoinPredicates);
			newChildNodes->Append(nAryJoinPredicateList);

			// some sanity checks

			// Example:  t1 join t2 on p12 left outer join t3 on p23 join t4 on p24 left outer join t5 on p35
			// results from this call:
			//    newChildNodes:       [ t1, t2, t3, t4, t5 ]
			//    lojChildPredIndexes: [  0,  0,  1,  0,  2 ] (one entry per logical leaf node)
			//    innerjoinPredicates: [ p12, p24 ]  (all correspond to child pred index 0 (GPOPT_ZERO_INNER_JOIN_PRED_INDEX))
			//    lojPredicates:       [ p23, p35 ]  (p23 corresponds to child pred index 1, p35 corresponds to child pred index 2)

			// the leftmost child must have a predicate index of
			// GPOPT_ZERO_INNER_JOIN_PRED_INDEX, since it cannot be the right child of an LOJ
			GPOS_ASSERT(GPOPT_ZERO_INNER_JOIN_PRED_INDEX ==
						*(*lojChildPredIndexes)[0]);

#ifdef GPOS_DEBUG
			// lojChildPredIndexes must contain the numbers 1 ... lojPredicates->Size()
			// in ascending order, each number exactly once, with optional additional
			// GPOPT_ZERO_INNER_JOIN_PRED_INDEX (0) entries in-between entries
			ULONG highestNumberSeen = 0;

			for (ULONG ix = 1; ix < lojChildPredIndexes->Size(); ix++)
			{
				ULONG nextNumber = *((*lojChildPredIndexes)[ix]);

				if (nextNumber == highestNumberSeen + 1)
				{
					// child is right child of an LOJ
					highestNumberSeen = nextNumber;
				}
				else
				{
					// if we don't see the next number for a child, it must
					// be associated with the collective inner join predicates
					GPOS_ASSERT(GPOPT_ZERO_INNER_JOIN_PRED_INDEX == nextNumber);
				}
			}
			GPOS_ASSERT(highestNumberSeen == lojPredicates->Size());
#endif
		}
		else
		{
			// no LOJs involved, just add the ANDed preds as the scalar child
			newChildNodes->Append(
				CPredicateUtils::PexprConjunction(mp, innerJoinPredicates));

			lojChildPredIndexes = nullptr;
		}

		gpos::Ref<CExpression> pexprNAryJoin = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalNAryJoin(mp, lojChildPredIndexes),
			newChildNodes);

		gpos::Ref<COptimizerConfig> optimizer_config =
			COptCtxt::PoctxtFromTLS()->GetOptimizerConfig();
		ULONG ulJoinArityLimit =
			optimizer_config->GetHint()
				->UlJoinArityForAssociativityCommutativity();

		// The last child of an n-ary join expression is the scalar expression
		if (pexprNAryJoin->Arity() - 1 > ulJoinArityLimit)
		{
			GPOPT_DISABLE_XFORM(CXform::ExfJoinCommutativity);
			GPOPT_DISABLE_XFORM(CXform::ExfJoinAssociativity);
		}


		return pexprNAryJoin;
	}
	// current operator is not an inner-join or supported LOJ, recursively process children
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprCollapseJoins(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// collect the children of a join backbone into an array of logical leaf
// nodes (leaves of the backbone, that is) and arrays of predicates, such
// that we can still associate the correct ON predicates to the children
void
CExpressionPreprocessor::CollectJoinChildrenRecursively(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CExpressionArray> logicalLeafNodes,
	gpos::Ref<ULongPtrArray> lojChildPredIndexes,
	gpos::Ref<CExpressionArray> innerJoinPredicates,
	gpos::Ref<CExpressionArray> lojPredicates)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(pexpr->Pop()->FLogical());

	if (CPredicateUtils::FInnerOrNAryJoin(pexpr))
	{
		const ULONG arity = pexpr->Arity();
		gpos::Ref<CExpression> pexprScalar = (*pexpr)[arity - 1];

		if (COperator::EopScalarNAryJoinPredList != pexprScalar->Pop()->Eopid())
		{
			for (ULONG ul = 0; ul < arity - 1; ul++)
			{
				gpos::Ref<CExpression> child = (*pexpr)[ul];
				CollectJoinChildrenRecursively(
					mp, child, logicalLeafNodes, lojChildPredIndexes,
					innerJoinPredicates, lojPredicates);
			}

			innerJoinPredicates->Append(PexprCollapseJoins(mp, pexprScalar));
		}
		else
		{
			// we have collapsed this join before and it already has some non-inner join info,
			// merge the existing and new lists
			gpos::Ref<CLogicalNAryJoin> naryJoin =
				CLogicalNAryJoin::PopConvert(pexpr->Pop());
			gpos::Ref<ULongPtrArray> naryJoinPredIndexes =
				naryJoin->GetLojChildPredIndexes();

			// add all the inner join predicates
			innerJoinPredicates->Append(
				PexprCollapseJoins(mp, (*pexprScalar)[0]));

			// loop over the logical children
			for (ULONG ul = 0; ul < arity - 1; ul++)
			{
				if (GPOPT_ZERO_INNER_JOIN_PRED_INDEX ==
					*(*naryJoinPredIndexes)[ul])
				{
					// inner join child, collapse recursively
					CollectJoinChildrenRecursively(
						mp, (*pexpr)[ul], logicalLeafNodes, lojChildPredIndexes,
						innerJoinPredicates, lojPredicates);
				}
				else
				{
					// this is the right child of a non-inner join
					ULONG oldPredIndex = *(*naryJoinPredIndexes)[ul];
					gpos::Ref<CExpression> lojPred =
						PexprCollapseJoins(mp, (*pexprScalar)[oldPredIndex]);

					// don't collapse this child into our current join node
					logicalLeafNodes->Append(
						PexprCollapseJoins(mp, (*pexpr)[ul]));
					lojPredicates->Append(lojPred);

					ULONG newPredIndex = lojPredicates->Size();

					lojChildPredIndexes->Append(GPOS_NEW(mp)
													ULONG(newPredIndex));
				}
			}
		}
	}
	else if (GPOS_FTRACE(EopttraceEnableLOJInNAryJoin) &&
			 CPredicateUtils::FLeftOuterJoin(pexpr))
	{
		GPOS_ASSERT(3 == pexpr->Arity());

		gpos::Ref<CExpression> leftChild = (*pexpr)[0];
		gpos::Ref<CExpression> rightChild = (*pexpr)[1];
		gpos::Ref<CExpression> pexprScalar = (*pexpr)[2];

		CollectJoinChildrenRecursively(mp, leftChild, logicalLeafNodes,
									   lojChildPredIndexes, innerJoinPredicates,
									   lojPredicates);

		// stop collecting join children at the right child of the LOJ,
		// just add the child, regardless of whether it is a join or not
		logicalLeafNodes->Append(PexprCollapseJoins(mp, rightChild));

		// create an entry in lojPredicates...
		lojPredicates->Append(PexprCollapseJoins(mp, pexprScalar));

		// ... and point to this new entry in lojChildPredIndexes
		ULONG *indexOfThisLOJInTheArray =
			GPOS_NEW(mp) ULONG(lojPredicates->Size());
		lojChildPredIndexes->Append(indexOfThisLOJInTheArray);
	}
	else
	{
		// pexpr is not the right child of a supported LOJ and is not a supported join
		logicalLeafNodes->Append(PexprCollapseJoins(mp, pexpr));

		// this logical "leaf" node is a child of an inner join or it is the left child
		// of an LOJ, either way it is associated with the inner join predicates
		ULONG *innerJoinPredIndex =
			GPOS_NEW(mp) ULONG(GPOPT_ZERO_INNER_JOIN_PRED_INDEX);
		lojChildPredIndexes->Append(innerJoinPredIndex);
	}
}


// collapse cascaded logical project operators
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprCollapseProjects(CMemoryPool *mp,
											   gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG arity = pexpr->Arity();
	// recursively process children
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprCollapseProjects(mp, (*pexpr)[ul]);
		pdrgpexpr->Append(pexprChild);
	}

	gpos::Ref<COperator> pop = pexpr->Pop();


	gpos::Ref<CExpression> pexprNew =
		GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
	gpos::Ref<CExpression> pexprCollapsed =
		CUtils::PexprCollapseProjects(mp, pexprNew);

	if (nullptr == pexprCollapsed)
	{
		return pexprNew;
	}



	return pexprCollapsed;
}

// insert dummy project element below scalar subquery when the (a) the scalar
// subquery is below a project and (b) output column is an outer reference
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprProjBelowSubquery(CMemoryPool *mp,
												gpos::Ref<CExpression> pexpr,
												BOOL fUnderPrList)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	/*
	 * Consider the following subquery:
	 * SELECT (SELECT foo.b from bar) FROM foo
	 * If bar is empty we should return null.
	 *
	 * For this query during DXL->Expr translation, the project element
	 * (SELECT b FROM bar) is represented as scalar subquery that returns
	 * an output column. To ensure that this scalar subquery under the project
	 * operator is returned when bar (or an arbitrary tree instead of bar)
	 * we insert a dummy project element that points to FOO.b under the
	 * scalar subquery. This dummy project element prevents its incorrect
	 * transformation into a non-correlated plan.
	 *
	 * One of the reasons we add this dummy project is to force the subquery
	 * handler transformation to not produce a de-correlated plan
	 * for queries such as this.
	 *
	 * We want to limit the of such introduction dummy projects only when the
	 * following conditions are all satisfied:
	 * a)  The scalar subquery is in the project element scalar tree
	 * Another use case: SELECT (SELECT foo.b from bar) + 1 FROM foo
	 * b) The output of the scalar subquery is the column from the outer expression.
	 * Consider the query: SELECT (SELECT foo.b + 5 from bar) FROM foo. In such cases,
	 * since the foo.b + 5 is a new computed column inside the subquery with its own
	 * project element, we do not need to add anything.
	 */
	BOOL fUnderPrListChild = fUnderPrList;
	gpos::Ref<COperator> pop = pexpr->Pop();

	if (pop->FLogical())
	{
		if (COperator::EopLogicalProject == pop->Eopid())
		{
			gpos::Ref<CExpression> pexprRel = (*pexpr)[0];
			gpos::Ref<CExpression> pexprRelNew =
				PexprProjBelowSubquery(mp, pexprRel, false /* fUnderPrList */);

			gpos::Ref<CExpression> pexprPrList = (*pexpr)[1];
			gpos::Ref<CExpression> pexprPrListNew = PexprProjBelowSubquery(
				mp, pexprPrList, true /* fUnderPrList */);

			return GPOS_NEW(mp)
				CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), pexprRelNew,
							pexprPrListNew);
		}

		fUnderPrListChild = false;
	}
	else if (COperator::EopScalarSubquery == pop->Eopid() && fUnderPrList)
	{
		gpos::Ref<CExpression> pexprRel = (*pexpr)[0];
		gpos::Ref<CExpression> pexprRelNew =
			PexprProjBelowSubquery(mp, pexprRel, false /* fUnderPrList */);

		const gpos::Ref<CColRefSet> prcsOutput =
			pexprRelNew->DeriveOutputColumns();
		const gpos::Ref<CColRef> pcrSubquery =
			CScalarSubquery::PopConvert(pop)->Pcr();
		if (nullptr != prcsOutput && !prcsOutput->FMember(pcrSubquery))
		{
			CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
			gpos::Ref<CColRef> pcrNewSubquery = col_factory->PcrCreate(
				pcrSubquery->RetrieveType(), pcrSubquery->TypeModifier());

			gpos::Ref<CExpression> pexprPrEl =
				CUtils::PexprScalarProjectElement(
					mp, pcrNewSubquery,
					CUtils::PexprScalarIdent(mp, pcrSubquery));
			gpos::Ref<CExpression> pexprProjList = GPOS_NEW(mp)
				CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), pexprPrEl);
			gpos::Ref<CExpression> pexprProj =
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp),
										 pexprRelNew, pexprProjList);

			gpos::Ref<CScalarSubquery> popSubq = GPOS_NEW(mp)
				CScalarSubquery(mp, pcrNewSubquery, false /*fGeneratedByExist*/,
								false /*fGeneratedByQuantified*/);

			gpos::Ref<CExpression> pexprResult =
				GPOS_NEW(mp) CExpression(mp, popSubq, pexprProj);
			return pexprResult;
		}


		return GPOS_NEW(mp) CExpression(mp, pop, pexprRelNew);
	}

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprProjBelowSubquery(mp, (*pexpr)[ul], fUnderPrListChild);
		pdrgpexpr->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// collapse cascaded union/union all into an NAry union/union all operator
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprCollapseUnionUnionAll(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	const ULONG arity = pexpr->Arity();

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	// recursively process children
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprCollapseUnionUnionAll(mp, (*pexpr)[ul]);
		pdrgpexpr->Append(pexprChild);
	}


	gpos::Ref<CExpression> pexprNew =
		GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
	if (!CPredicateUtils::FUnionOrUnionAll(pexprNew))
	{
		return pexprNew;
	}

	// array of input children and its column references
	gpos::Ref<CExpressionArray> pdrgpexprNew =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CColRef2dArray> pdrgdrgpcrOrig =
		CLogicalSetOp::PopConvert(pop)->PdrgpdrgpcrInput();
	gpos::Ref<CColRef2dArray> pdrgdrgpcrNew = GPOS_NEW(mp) CColRef2dArray(mp);

	BOOL fCollapsed = false;
	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (CPredicateUtils::FCollapsibleChildUnionUnionAll(pexprNew, ul))
		{
			fCollapsed = true;
			CPredicateUtils::CollectGrandChildrenUnionUnionAll(
				mp, pexprNew, ul, pdrgpexprNew, pdrgdrgpcrNew);
		}
		else
		{
			gpos::Ref<CExpression> pexprChild = (*pexprNew)[ul];

			pdrgpexprNew->Append(pexprChild);

			gpos::Ref<CColRefArray> pdrgpcrInput = (*pdrgdrgpcrOrig)[ul];

			pdrgdrgpcrNew->Append(pdrgpcrInput);
		}
	}

	if (!fCollapsed)
	{
		// clean up



		return pexprNew;
	}

	gpos::Ref<COperator> popNew = nullptr;
	gpos::Ref<CColRefArray> pdrgpcrOutput =
		CLogicalSetOp::PopConvert(pop)->PdrgpcrOutput();

	if (pop->Eopid() == COperator::EopLogicalUnion)
	{
		popNew = GPOS_NEW(mp) CLogicalUnion(mp, pdrgpcrOutput, pdrgdrgpcrNew);
	}
	else
	{
		GPOS_ASSERT(pop->Eopid() == COperator::EopLogicalUnionAll);
		popNew =
			GPOS_NEW(mp) CLogicalUnionAll(mp, pdrgpcrOutput, pdrgdrgpcrNew);
	}

	// clean up


	return GPOS_NEW(mp) CExpression(mp, popNew, pdrgpexprNew);
}

// transform outer joins into inner joins
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprOuterJoinToInnerJoin(CMemoryPool *mp,
												   gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	const ULONG arity = pexpr->Arity();

	if (COperator::EopLogicalSelect == pop->Eopid() &&
		COperator::EopLogicalLeftOuterJoin == (*pexpr)[0]->Pop()->Eopid())
	{
		// a Select on top of LOJ can be turned into InnerJoin by normalization
		return CNormalizer::PexprNormalize(mp, pexpr);
	}

	if (CPredicateUtils::FInnerOrNAryJoin(pexpr))
	{
		// the predicates of an inner join on top of outer join can be used to turn the child outer join into another inner join
		gpos::Ref<CExpression> pexprScalar = (*pexpr)[arity - 1];

		if (COperator::EopScalarNAryJoinPredList == pexprScalar->Pop()->Eopid())
		{
			// since we have ScalarNAryJoinPredList, it means we have already
			// converted all possible LOJs to Inner Joins and collapsed them

			return pexpr;
		}

		gpos::Ref<CExpressionArray> pdrgpexprChildren =
			GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ul = 0; ul < arity; ul++)
		{
			gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
			BOOL fNewChild = false;
			if (COperator::EopLogicalLeftOuterJoin ==
				pexprChild->Pop()->Eopid())
			{
				gpos::Ref<CColRefSet> pcrsLOJInnerOutput =
					(*pexprChild)[1]->DeriveOutputColumns();
				if (!GPOS_FTRACE(EopttraceDisableOuterJoin2InnerJoinRewrite) &&
					CPredicateUtils::FNullRejecting(mp, pexprScalar,
													pcrsLOJInnerOutput))
				{
					gpos::Ref<CExpression> pexprNewOuter =
						PexprOuterJoinToInnerJoin(mp, (*pexprChild)[0]);
					gpos::Ref<CExpression> pexprNewInner =
						PexprOuterJoinToInnerJoin(mp, (*pexprChild)[1]);
					gpos::Ref<CExpression> pexprNewScalar =
						PexprOuterJoinToInnerJoin(mp, (*pexprChild)[2]);
					pexprChild = CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
						mp, pexprNewOuter, pexprNewInner, pexprNewScalar);
					fNewChild = true;
				}
			}

			// Consider the following join tree:
			// +--CLogicalNAryJoin
			// |--CLogicalLeftOuterJoin
			// |  |--CLogicalLeftOuterJoin
			// |  |  |--CLogicalGet "t1"
			// |  |  |--CLogicalGet "t2"
			// |  |--CLogicalGet "t3"
			// |--CLogicalGet "t4"
			//
			// If the predicate between the CLogicalNAryJoin and first CLogicalLeftOuterJoin
			// is NULL rejecting, we convert the left join to an inner join and create a new
			// expression. Then the modified tree would be:
			//
			// +--CLogicalNAryJoin
			// |--CLogicalLeftOuterJoin
			// |  |--CLogicalGet "t1"
			// |  |--CLogicalGet "t2"
			// |--CLogicalGet "t3"
			// |--CLogicalGet "t4"
			//
			// Note that we can still convert the second CLogicalLeftOuterJoin into an inner join
			// if the predicate between the CLogicalNAryJoin is NULL rejecting. So we need to recurse
			// into the child and continue checking if we can convert the LOJs into inner joins.

			gpos::Ref<CExpression> pexprChildNew =
				PexprOuterJoinToInnerJoin(mp, pexprChild);
			if (fNewChild)
			{
			}
			pdrgpexprChildren->Append(pexprChildNew);
		}

		return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalNAryJoin(mp),
										pdrgpexprChildren);
	}

	// current operator is not an NAry-join, recursively process children
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprOuterJoinToInnerJoin(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// generate n*(n-1)/2 equality predicates, up to GPOPT_MAX_DERIVED_PREDS, between
// the n columns in the given equivalence class (set),
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprConjEqualityPredicates(CMemoryPool *mp,
													 gpos::Ref<CColRefSet> pcrs)
{
	GPOS_ASSERT(nullptr != pcrs);

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	ULONG ulPreds = 0;
	CColRefSetIter crsiRight(*pcrs);
	while (crsiRight.Advance() && GPOPT_MAX_DERIVED_PREDS > ulPreds)
	{
		gpos::Ref<CColRef> pcrRight = crsiRight.Pcr();

		CColRefSetIter crsiLeft(*pcrs);
		while (crsiLeft.Advance() && GPOPT_MAX_DERIVED_PREDS > ulPreds)
		{
			gpos::Ref<CColRef> pcrLeft = crsiLeft.Pcr();
			if (pcrLeft == pcrRight)
			{
				break;
			}

			pdrgpexpr->Append(CUtils::PexprScalarEqCmp(mp, pcrLeft, pcrRight));
			ulPreds++;
		}
	}

	return CPredicateUtils::PexprConjunction(mp, pdrgpexpr);
}

// check if all columns in the given equivalent class come from one of the
// children of the given expression
BOOL
CExpressionPreprocessor::FEquivClassFromChild(gpos::Ref<CColRefSet> pcrs,
											  gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pcrs);
	GPOS_ASSERT(nullptr != pexpr);

	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		if (!pexprChild->Pop()->FLogical())
		{
			continue;
		}
		gpos::Ref<CColRefSetArray> pdrgpcrs =
			pexprChild->DerivePropertyConstraint()->PdrgpcrsEquivClasses();
		if (pcrs->FContained(pdrgpcrs))
		{
			return true;
		}
	}

	return false;
}

// additional equality predicates are generated based on the equivalence
// classes in the constraint properties of the expression.
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprAddEqualityPreds(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefSet> pcrsProcessed)
{
	GPOS_ASSERT(nullptr != pcrsProcessed);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(pexpr->Pop()->FLogical());

	const ULONG ulChildren = pexpr->Arity();
	gpos::Ref<CPropConstraint> ppc = pexpr->DerivePropertyConstraint();

	gpos::Ref<CExpression> pexprPred = nullptr;
	gpos::Ref<COperator> pop = pexpr->Pop();
	if (CUtils::FLogicalDML(pop))
	{
		pexprPred = CUtils::PexprScalarConstBool(mp, true);
	}
	else
	{
		gpos::Ref<CExpressionArray> pdrgpexpr =
			GPOS_NEW(mp) CExpressionArray(mp);
		gpos::Ref<CColRefSetArray> pdrgpcrs = ppc->PdrgpcrsEquivClasses();
		GPOS_ASSERT(nullptr != pdrgpcrs);
		const ULONG ulEquivClasses = pdrgpcrs->Size();
		for (ULONG ul = 0; ul < ulEquivClasses; ul++)
		{
			gpos::Ref<CColRefSet> pcrsEquivClass = (*pdrgpcrs)[ul];

			gpos::Ref<CColRefSet> pcrsEquality = GPOS_NEW(mp) CColRefSet(mp);
			pcrsEquality->Include(pcrsEquivClass);
			pcrsEquality->Exclude(pcrsProcessed);

			// if equivalence class comes from any of the children, then skip it
			if (FEquivClassFromChild(pcrsEquality, pexpr))
			{
				continue;
			}

			gpos::Ref<CExpression> pexprEquality =
				PexprConjEqualityPredicates(mp, pcrsEquality);
			pcrsProcessed->Include(pcrsEquality);

			pdrgpexpr->Append(pexprEquality);
		}

		pexprPred = CPredicateUtils::PexprConjunction(mp, pdrgpexpr);
	}

	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		if (pexprChild->Pop()->FLogical())
		{
			gpos::Ref<CExpression> pexprChildNew =
				PexprAddEqualityPreds(mp, pexprChild, pcrsProcessed);
			pdrgpexprChildren->Append(pexprChildNew);
		}
		else
		{
			pdrgpexprChildren->Append(pexprChild);
		}
	}



	return CUtils::PexprSafeSelect(
		mp, GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren), pexprPred);
}

// generate predicates for the given set of columns based on the given
// constraint property. Columns for which predicates are generated will be
// added to the set of processed columns
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprScalarPredicates(
	CMemoryPool *mp, gpos::Ref<CPropConstraint> ppc,
	gpos::Ref<CPropConstraint> constraintsForOuterRefs,
	gpos::Ref<CPropConstraint> ppcFromFilterSubquery,
	gpos::Ref<CColRefSet> pcrsNotNull, gpos::Ref<CColRefSet> pcrs,
	gpos::Ref<CColRefSet> pcrsProcessed)
{
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	CColRefSetIter crsi(*pcrs);
	while (crsi.Advance())
	{
		gpos::Ref<CColRef> colref = crsi.Pcr();
		gpos::Ref<CColRefSet> crs = nullptr;  // equiv class column refset

		if (ppcFromFilterSubquery != nullptr &&
			(crs = ppcFromFilterSubquery->PcrsEquivClass(colref)) != nullptr)
		{
			// add predicates that are inferred from subquery
			gpos::Ref<CConstraint> pcnstr =
				ppcFromFilterSubquery->Pcnstr()->Pcnstr(mp, crs);
			gpos::Ref<CConstraint> pcnstrCol =
				pcnstr->PcnstrRemapForColumn(mp, colref);
			gpos::Ref<CExpression> pexprScalar = pcnstrCol->PexprScalar(mp);

			// do not add a NOT NULL predicate if column is not nullable or if it
			// already has another predicate on it
			if (!(CUtils::FScalarNotNull(pexprScalar) &&
				  (pcrsNotNull->FMember(colref) ||
				   (ppc->Pcnstr() != nullptr &&
					ppc->Pcnstr()->FConstraint(colref)))))
			{
				pdrgpexpr->Append(pexprScalar);
			}

			pcrsProcessed->Include(colref);
		}

		gpos::Ref<CExpression> pexprScalar =
			ppc->PexprScalarMappedFromEquivCols(mp, colref,
												constraintsForOuterRefs);
		if (nullptr == pexprScalar)
		{
			continue;
		}

		// do not add a NOT NULL predicate if column is not nullable or if it
		// already has another predicate on it
		if (CUtils::FScalarNotNull(pexprScalar) &&
			(pcrsNotNull->FMember(colref) ||
			 ppc->Pcnstr()->FConstraint(colref)))
		{
			continue;
		}
		pdrgpexpr->Append(pexprScalar);
		// set the column `processed` after generate new predicate on this column.
		pcrsProcessed->Include(colref);
	}

	if (0 == pdrgpexpr->Size())
	{
		return nullptr;
	}

	return CPredicateUtils::PexprConjunction(mp, pdrgpexpr);
}

// process scalar expressions for generating additional predicates based on
// derived constraints. This function is needed because scalar expressions
// can have relational children when there are subqueries
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprFromConstraintsScalar(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CPropConstraint> constraintsForOuterRefs)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(pexpr->Pop()->FScalar());

	if (!CUtils::FHasSubquery(pexpr))
	{
		return pexpr;
	}

	const ULONG ulChildren = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		if (pexprChild->Pop()->FScalar())
		{
			pexprChild = PexprFromConstraintsScalar(mp, pexprChild,
													constraintsForOuterRefs);
		}
		else
		{
			GPOS_ASSERT(pexprChild->Pop()->FLogical());
			gpos::Ref<CColRefSet> pcrsProcessed = GPOS_NEW(mp) CColRefSet(mp);
			pexprChild = PexprFromConstraints(mp, pexprChild, pcrsProcessed,
											  constraintsForOuterRefs);
		}

		pdrgpexprChildren->Append(pexprChild);
	}

	gpos::Ref<COperator> pop = pexpr->Pop();

	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// Imply new predicates on LOJ's inner child based on constraints derived
// from LOJ's outer child and join predicate
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprWithImpliedPredsOnLOJInnerChild(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprLOJ,
	BOOL *
		pfAddedPredicates  // output: set to True if new predicates are added to inner child
)
{
	GPOS_ASSERT(nullptr != pexprLOJ);
	GPOS_ASSERT(nullptr != pfAddedPredicates);
	GPOS_ASSERT(COperator::EopLogicalLeftOuterJoin == pexprLOJ->Pop()->Eopid());

	gpos::Ref<CExpression> pexprOuter = (*pexprLOJ)[0];
	gpos::Ref<CExpression> pexprInner = (*pexprLOJ)[1];
	gpos::Ref<CExpression> pexprOuterJoinPred = (*pexprLOJ)[2];

	// merge children constraints with constraints derived from join's predicate
	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexprLOJ);
	gpos::Ref<CPropConstraint> ppc =
		CLogical::PpcDeriveConstraintFromPredicates(mp, exprhdl);

	// use the computed constraint to derive a scalar predicate on the inner child
	gpos::Ref<CColRefSet> pcrsInnerOutput = pexprInner->DeriveOutputColumns();
	gpos::Ref<CColRefSet> pcrsInnerNotNull = pexprInner->DeriveNotNullColumns();

	// generate a scalar predicate from the computed constraint, restricted to LOJ inner child
	gpos::Ref<CColRefSet> pcrsProcessed = GPOS_NEW(mp) CColRefSet(mp);
	gpos::Ref<CExpression> pexprPred =
		PexprScalarPredicates(mp, ppc, nullptr /*no outer refs*/,
							  nullptr /*no constraints from subquery*/,
							  pcrsInnerNotNull, pcrsInnerOutput, pcrsProcessed);



	if (nullptr != pexprPred && !CUtils::FScalarConstTrue(pexprPred))
	{
		// if a new predicate was added, set the output flag to True
		*pfAddedPredicates = true;

		gpos::Ref<CExpression> pexprSelect =
			CUtils::PexprLogicalSelect(mp, pexprInner, pexprPred);
		gpos::Ref<CExpression> pexprInnerNormalized =
			CNormalizer::PexprNormalize(mp, pexprSelect);

		pexprInner = pexprInnerNormalized;
	}


	// recursively process inner child
	gpos::Ref<CExpression> pexprNewInner =
		PexprOuterJoinInferPredsFromOuterChildToInnerChild(mp, pexprInner,
														   pfAddedPredicates);


	// recursively process outer child
	gpos::Ref<CExpression> pexprNewOuter =
		PexprOuterJoinInferPredsFromOuterChildToInnerChild(mp, pexprOuter,
														   pfAddedPredicates);


	gpos::Ref<COperator> pop = pexprLOJ->Pop();


	return GPOS_NEW(mp)
		CExpression(mp, pop, pexprNewOuter, pexprNewInner, pexprOuterJoinPred);
}

// Infer predicate from outer child to inner child of the outer join,
//
//	for LOJ expressions with predicates on outer child, e.g.,
//
//		+-LOJ(x=y)
//  		|---Select(x=5)
// 	    	|   	+----X
// 	   		+----Y
//
//	this function implies an equivalent predicate (y=5) on the inner child of LOJ:
//
//		+-LOJ(x=y)
//			|---Select(x=5)
//			|		+----X
//			+---Select(y=5)
//					+----Y
//
//	the correctness of this rewrite can be proven as follows:
//		- By removing all tuples from Y that do not satisfy (y=5), the LOJ
//		results, where x=y, are retained. The reason is that any such join result
//		must satisfy (x=5 ^ x=y) which implies that (y=5).
//
//		- LOJ results that correspond to tuples from X not joining with any tuple
//		from Y are also retained. The reason is that such join results can only be
//		produced if for all tuples in Y, we have (y!=5). By selecting Y tuples where (y=5),
//		if we end up with no Y tuples, the LOJ results will be generated by joining X with empty Y.
//		This is the same as joining with all tuples from Y with (y!=5). If we end up with
//		any tuple in Y satisfying (y=5), no LOJ results corresponding to X tuples not joining
//		with Y can be produced.
//
//	to implement this rewrite in a general form, we need to imply general constraints on
//	LOJ's inner child from constraints that exist on LOJ's outer child. The generated predicate
//	from this inference can only be inserted below LOJ (on top of the inner child), and cannot be
//	inserted on top of LOJ, otherwise we may wrongly convert LOJ to inner-join.
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprOuterJoinInferPredsFromOuterChildToInnerChild(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	BOOL *
		pfAddedPredicates  // output: set to True if new predicates are added to inner child
)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pfAddedPredicates);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (COperator::EopLogicalLeftOuterJoin == pop->Eopid())
	{
		return PexprWithImpliedPredsOnLOJInnerChild(mp, pexpr,
													pfAddedPredicates);
	}

	// not an outer join, process children recursively
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprOuterJoinInferPredsFromOuterChildToInnerChild(
				mp, (*pexpr)[ul], pfAddedPredicates);
		pdrgpexpr->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// additional predicates are generated based on the derived constraint
// properties of the expression. No predicates are generated for the columns
// in the already processed set. This set is expanded with more columns
// that get processed along the way
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprFromConstraints(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefSet> pcrsProcessed,
	gpos::Ref<CPropConstraint> constraintsForOuterRefs)
{
	GPOS_ASSERT(nullptr != pcrsProcessed);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(pexpr->Pop()->FLogical());

	const ULONG ulChildren = pexpr->Arity();
	gpos::Ref<CPropConstraint> ppc = pexpr->DerivePropertyConstraint();
	gpos::Ref<CColRefSet> pcrsNotNull = pexpr->DeriveNotNullColumns();
	gpos::Ref<CPropConstraint> ppcFromFilterSubquery =
		CExpressionUtils::GetPropConstraintFromSubquery(mp, pexpr);
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		if (pexprChild->Pop()->FScalar())
		{
			// we could potentially combine constraintsForOuterRefs and ppc,
			// but for now just pass ppc, the constraints from the direct ancestor
			// of the subquery
			pexprChild = PexprFromConstraintsScalar(mp, pexprChild, ppc);
			pdrgpexprChildren->Append(pexprChild);
			continue;
		}

		// process child
		gpos::Ref<CExpression> pexprChildNew = PexprFromConstraints(
			mp, pexprChild, pcrsProcessed, constraintsForOuterRefs);

		gpos::Ref<CColRefSet> pcrsOutChild = GPOS_NEW(mp) CColRefSet(mp);
		// output columns on which predicates must be inferred
		pcrsOutChild->Include(pexprChild->DeriveOutputColumns());

		// exclude column references on which predicates had been already inferred,
		// this avoids generating duplicate predicates on the parent node if a
		// predicate has already been placed on the child.
		pcrsOutChild->Exclude(pcrsProcessed);

		// generate predicates for the output columns of child
		gpos::Ref<CExpression> pexprPred = PexprScalarPredicates(
			mp, ppc, constraintsForOuterRefs, ppcFromFilterSubquery,
			pcrsNotNull, pcrsOutChild, pcrsProcessed);


		if (nullptr != pexprPred)
		{
			pdrgpexprChildren->Append(
				CUtils::PexprSafeSelect(mp, pexprChildNew, pexprPred));
		}
		else
		{
			pdrgpexprChildren->Append(pexprChildNew);
		}
	}

	gpos::Ref<COperator> pop = pexpr->Pop();


	gpos::Ref<CExpression> pexprPred =
		GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
	if (ppcFromFilterSubquery != nullptr)
	{
		gpos::Ref<CExpression> pexprNormalized =
			CNormalizer::PexprNormalize(mp, pexprPred);

		pexprPred = pexprNormalized;
	}

	return pexprPred;
}

// eliminate subtrees that have a zero output cardinality, replacing them
// with a const table get with the same output schema and zero tuples
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprPruneEmptySubtrees(CMemoryPool *mp,
												 gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (pop->FLogical() && !CUtils::FLogicalDML(pop))
	{
		// if maxcard = 0: return a const table get with same output columns and zero tuples
		if (0 == pexpr->DeriveMaxCard())
		{
			// output columns
			gpos::Ref<CColRefArray> colref_array =
				pexpr->DeriveOutputColumns()->Pdrgpcr(mp);

			// empty output data
			gpos::Ref<IDatum2dArray> pdrgpdrgpdatum =
				GPOS_NEW(mp) IDatum2dArray(mp);

			gpos::Ref<COperator> popCTG = GPOS_NEW(mp)
				CLogicalConstTableGet(mp, colref_array, pdrgpdrgpdatum);
			return GPOS_NEW(mp) CExpression(mp, popCTG);
		}
	}

	// process children
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprPruneEmptySubtrees(mp, (*pexpr)[ul]);
		pdrgpexpr->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// eliminate CTE Anchors for CTEs that have zero consumers
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprRemoveUnusedCTEs(CMemoryPool *mp,
											   gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (COperator::EopLogicalCTEAnchor == pop->Eopid())
	{
		ULONG id = CLogicalCTEAnchor::PopConvert(pop)->Id();
		if (!COptCtxt::PoctxtFromTLS()->Pcteinfo()->FUsed(id))
		{
			GPOS_ASSERT(1 == pexpr->Arity());
			return PexprRemoveUnusedCTEs(mp, (*pexpr)[0]);
		}
	}

	// process children
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprRemoveUnusedCTEs(mp, (*pexpr)[ul]);
		pdrgpexpr->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// Create an identifier to constant map
//
// Given a select filter expression, map all conjunctive equality between an
// identifier and a constant value.
//
// NB: This function either inserts or deletes mapped values. Parameter
//     'doInsert' is used to indicate which of the two to perform.
static void
UpdateExprToConstantPredicateMapping(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprFilter,
	gpos::Ref<ExprToConstantMap> phmExprToConst, BOOL doInsert)
{
	if (CPredicateUtils::IsEqualityOp(pexprFilter))
	{
		// the following section handles predicates of the kind (a = 10)
		if (!CUtils::FScalarConstOrBinaryCoercible((*pexprFilter)[0]) &&
			CUtils::FScalarConstOrBinaryCoercible((*pexprFilter)[1]))
		{
			if (doInsert)
			{
				phmExprToConst->Insert((*pexprFilter)[0], (*pexprFilter)[1]);
			}
			else
			{
				phmExprToConst->Delete((*pexprFilter)[0]);
			}
		}
		// the following section handles predicates of the kind (10 = a)
		else if (CUtils::FScalarConstOrBinaryCoercible((*pexprFilter)[0]) &&
				 !CUtils::FScalarConstOrBinaryCoercible((*pexprFilter)[1]))
		{
			if (doInsert)
			{
				phmExprToConst->Insert((*pexprFilter)[1], (*pexprFilter)[0]);
			}
			else
			{
				phmExprToConst->Delete((*pexprFilter)[1]);
			}
		}
	}
	else if (CPredicateUtils::FAnd(pexprFilter))
	{
		const ULONG ulChildren = pexprFilter->Arity();
		for (ULONG ul = 0; ul < ulChildren; ul++)
		{
			UpdateExprToConstantPredicateMapping(mp, (*pexprFilter)[ul],
												 phmExprToConst, doInsert);
		}
	}
}

// Create a new expression from an exression and map of ident to const
static gpos::Ref<CExpression>
SubstituteConstantIdentifier(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							 gpos::Ref<ExprToConstantMap> phmExprToConst)
{
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild = nullptr;
		if (COperator::EopScalarConst != (*pexpr)[ul]->Pop()->Eopid() &&
			phmExprToConst->Find((*pexpr)[ul]) != nullptr &&
			// make sure the types match
			CScalar::PopConvert(phmExprToConst->Find((*pexpr)[ul])->Pop())
				->MdidType()
				->Equals(CScalar::PopConvert((*pexpr)[ul]->Pop())->MdidType()))
		{
			// substitute with constant
			pexprChild = phmExprToConst->Find((*pexpr)[ul]);
		}
		else
		{
			pexprChild =
				SubstituteConstantIdentifier(mp, (*pexpr)[ul], phmExprToConst);
		}
		pdrgpexpr->Append(pexprChild);
	}

	gpos::Ref<COperator> pop = pexpr->Pop();

	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// Apply filter that replaces constants used in join predicate
//
// This preprocessing step enables additional opportunities for predicate push
// down based on synergetic join conditions and select filters.  For example,
// let's use query:
//
//     SELECT count(*) FROM t2 INNER JOIN t1 ON t2.c BETWEEN t1.a AND t1.b WHERE t2.c = 2;
//
// Input:
// +--CLogicalSelect
//    |--CLogicalNAryJoin
//    |  |--CLogicalGet "t2" ("t2"), Columns: ["c" (0), "d" (1),...
//    |  |--CLogicalGet "t1" ("t1"), Columns: ["a" (9), "b" (10),...
//    |  +--CScalarBoolOp (EboolopAnd)
//    |     |--CScalarCmp (>=)
//    |     |  |--CScalarIdent "c" (0)
//    |     |  +--CScalarIdent "a" (9)
//    |     +--CScalarCmp (<=)
//    |        |--CScalarIdent "c" (0)
//    |        +--CScalarIdent "b" (10)
//    +--CScalarCmp (=)
//       |--CScalarIdent "c" (0)
//       +--CScalarConst (2)
//
// Notice that the join predicate references columns (a, b, and c) coming from
// both sides of the join. Typically this means the predicate cannot be pushed
// down.  However, the select filter above the join predicate filters on c=2.
// We can utilize that in conjunction with the join condition to filter out
// more rows. We do that outputing:
//
// Output:
// +--CLogicalSelect
//    +--CLogicalSelect
//    |  |--CLogicalNAryJoin
//    |  |  |--CLogicalGet "t2" ("t2"), Columns: ["c" (0), "d" (1),...
//    |  |  |--CLogicalGet "t1" ("t1"), Columns: ["a" (9), "b" (10),...
//    |  |  +--CScalarBoolOp (EboolopAnd)
//    |  |     |--CScalarCmp (>=)
//    |  |     |  |--CScalarIdent "c" (0)
//    |  |     |  +--CScalarIdent "a" (9)
//    |  |     +--CScalarCmp (<=)
//    |  |        |--CScalarIdent "c" (0)
//    |  |        +--CScalarIdent "b" (10)
//    |  +--CScalarBoolOp (EboolopAnd)
//    |     |--CScalarCmp (>=)
//    |     |  +--CScalarConst (2)
//    |     |  +--CScalarIdent "a" (9)
//    |     +--CScalarCmp (<=)
//    |     |  +--CScalarConst (2)
//    |        +--CScalarIdent "b" (10)
//    +--CScalarCmp (=)
//       |--CScalarIdent "c" (0)
//       +--CScalarConst (2)
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprReplaceColWithConst(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<ExprToConstantMap> phmExprToConst, BOOL checkFilterForConstants)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	// Here we check for following pattern:
	//
	//     Select
	//       |-- NaryJoin/LeftOuterJoin
	//       |-- ...
	//       +-- Filter
	//
	// Notice that Select is processed twice with different values of
	// checkFilterForConstants. First time is to create the constant mapping.
	// The second time is to use the const mapping to process the node and
	// children to add additional select filters.
	if (checkFilterForConstants &&
		COperator::EopLogicalSelect == pexpr->Pop()->Eopid() &&
		(COperator::EopLogicalLeftOuterJoin == ((*pexpr)[0])->Pop()->Eopid() ||
		 COperator::EopLogicalNAryJoin == ((*pexpr)[0])->Pop()->Eopid()))
	{
		UpdateExprToConstantPredicateMapping(mp, (*pexpr)[pexpr->Arity() - 1],
											 phmExprToConst, true);

		gpos::Ref<CExpression> pexprNew =
			PexprReplaceColWithConst(mp, pexpr, phmExprToConst, false);

		// erase values from map...
		UpdateExprToConstantPredicateMapping(mp, (*pexpr)[pexpr->Arity() - 1],
											 phmExprToConst, false);

		return pexprNew;
	}

	// process children
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprReplaceColWithConst(mp, (*pexpr)[ul], phmExprToConst, true);
		pdrgpexpr->Append(pexprChild);
	}

	if (COperator::EopLogicalNAryJoin == pexpr->Pop()->Eopid() &&
		phmExprToConst->Size() > 0)
	{
		gpos::Ref<CExpression> pexprFilter = SubstituteConstantIdentifier(
			mp, (*pexpr)[pexpr->Arity() - 1], phmExprToConst);


		return GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalSelect(mp),
			GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr), pexprFilter);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// for all consumers of the same CTE, collect all selection predicates
// on top of these consumers, if any, and store them in hash map
void
CExpressionPreprocessor::CollectCTEPredicates(CMemoryPool *mp,
											  gpos::Ref<CExpression> pexpr,
											  CTEPredsMap *phm)
{
	GPOS_CHECK_STACK_SIZE;

	if (COperator::EopLogicalSelect == pexpr->Pop()->Eopid() &&
		COperator::EopLogicalCTEConsumer == (*pexpr)[0]->Pop()->Eopid() &&
		0 == pexpr->DeriveOuterReferences()
				 ->Size()  // no outer references in selection predicate
	)
	{
		gpos::Ref<CExpression> pexprScalar = (*pexpr)[1];
		if (!pexprScalar->DeriveHasSubquery())
		{
			gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
			gpos::Ref<CLogicalCTEConsumer> popConsumer =
				CLogicalCTEConsumer::PopConvert(pexprChild->Pop());
			ULONG ulCTEId = popConsumer->UlCTEId();
			gpos::Ref<CExpression> pexprProducer =
				COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(
					ulCTEId);
			GPOS_ASSERT(nullptr != pexprProducer);

			gpos::Ref<CLogicalCTEProducer> popProducer =
				CLogicalCTEProducer::PopConvert(pexprProducer->Pop());
			gpos::Ref<UlongToColRefMap> colref_mapping = CUtils::PhmulcrMapping(
				mp, popConsumer->Pdrgpcr(), popProducer->Pdrgpcr());
			gpos::Ref<CExpression> pexprRemappedScalar =
				pexprScalar->PexprCopyWithRemappedColumns(mp, colref_mapping,
														  true /*must_exist*/);


			gpos::Ref<CExpressionArray> pdrgpexpr = phm->Find(&ulCTEId);
			if (nullptr == pdrgpexpr)
			{
				pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
				BOOL fInserted GPOS_ASSERTS_ONLY =
					phm->Insert(GPOS_NEW(mp) ULONG(ulCTEId), pdrgpexpr);
				GPOS_ASSERT(fInserted);
			}
			pdrgpexpr->Append(pexprRemappedScalar);
		}
	}

	// process children recursively
	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		CollectCTEPredicates(mp, (*pexpr)[ul], phm);
	}
}

// add CTE predicates collected from consumers to producer expressions
void
CExpressionPreprocessor::AddPredsToCTEProducers(CMemoryPool *mp,
												gpos::Ref<CExpression> pexpr)
{
	CTEPredsMap *phm = GPOS_NEW(mp) CTEPredsMap(mp);
	CollectCTEPredicates(mp, pexpr, phm);

	gpos::Ref<CCTEInfo> pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	CTEPredsMapIter mi(phm);
	while (mi.Advance())
	{
		ULONG ulCTEId = *(mi.Key());
		gpos::Ref<CExpression> pexprProducer =
			pcteinfo->PexprCTEProducer(ulCTEId);
		GPOS_ASSERT(nullptr != pexprProducer);

		ULONG ulConsumers = pcteinfo->UlConsumers(ulCTEId);
		gpos::Ref<CExpressionArray> pdrgpexpr = mi.Value();

		// skip the propagation of predicate contains volatile function e.g. random() (value change within a scan)
		if (CPredicateUtils::FContainsVolatileFunction(pdrgpexpr))
		{
			continue;
		}

		if (0 < ulConsumers && pdrgpexpr->Size() == ulConsumers)
		{
			// add new predicate to CTE producer only if all consumers have selection predicates,
			// for example, in the following query
			// 'with v as (select * from A) select * from v where a > 5 union select * from v where b > 5'
			// we add the new predicate '(a > 5 or b > 5)' to CTE producer expression,
			// while for the following query
			// 'with v as (select * from A) select * from v where a > 5 union select * from v'
			// we do not add any new predicates to CTE producer expression


			gpos::Ref<CExpression> pexprPred =
				CPredicateUtils::PexprDisjunction(mp, pdrgpexpr);

			gpos::Ref<CExpression> pexprSelect =
				CUtils::PexprLogicalSelect(mp, (*pexprProducer)[0], pexprPred);
			gpos::Ref<COperator> pop = pexprProducer->Pop();

			gpos::Ref<CExpression> pexprNewProducer =
				GPOS_NEW(mp) CExpression(mp, pop, pexprSelect);

			pcteinfo->ReplaceCTEProducer(pexprNewProducer);
		}
	}
}

// derive constraints on given expression tree, and add new predicates by implication
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprAddPredicatesFromConstraints(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// normalize the tree, push down predicates (since we infer predicates bottom-up,
	// we want the predicates/constraints to be at the lowest possible point in the tree)
	gpos::Ref<CExpression> pexprNormalized =
		CNormalizer::PexprNormalize(mp, pexpr);

	// walk the tree and generate additional predicates from constraint properties
	// based on equivalence classes, e.g. constraint a=1 and equiv class {a,b} adds pred b=1
	gpos::Ref<CColRefSet> pcrsProcessed = GPOS_NEW(mp) CColRefSet(mp);
	gpos::Ref<CExpression> pexprConstraints =
		PexprFromConstraints(mp, pexprNormalized, pcrsProcessed, nullptr);
	GPOS_CHECK_ABORT;



	// walk the tree again and generate equality predicates for columns in
	// equivalence classes, e.g. {cr1,cr2,cr3} results in cr1=cr2 and cr1=cr3 and cr2=cr3
	pcrsProcessed = GPOS_NEW(mp) CColRefSet(mp);
	gpos::Ref<CExpression> pexprAddEqualityPreds =
		PexprAddEqualityPreds(mp, pexprConstraints, pcrsProcessed);

	// normalize the tree, push down predicates
	gpos::Ref<CExpression> pexprEqualityNormalized =
		CNormalizer::PexprNormalize(mp, pexprAddEqualityPreds);
	GPOS_CHECK_ABORT;



	// remove generated duplicate predicates
	gpos::Ref<CExpression> pexprDeduped =
		CExpressionUtils::PexprDedupChildren(mp, pexprEqualityNormalized);


	return pexprDeduped;
}

// driver for inferring predicates from constraints
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprInferPredicates(CMemoryPool *mp,
											  gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	// generate new predicates from constraint properties and normalize the result
	gpos::Ref<CExpression> pexprWithPreds =
		PexprAddPredicatesFromConstraints(mp, pexpr);

	// infer predicates from outer child to inner child of outer join
	BOOL fNewPreds = false;
	gpos::Ref<CExpression> pexprInferredPreds =
		PexprOuterJoinInferPredsFromOuterChildToInnerChild(mp, pexprWithPreds,
														   &fNewPreds);

	pexprWithPreds = pexprInferredPreds;

	if (fNewPreds)
	{
		// if we succeeded in generating new predicates below outer join, we need to
		// re-derive constraints to generate any other potential predicates
		pexprWithPreds =
			PexprAddPredicatesFromConstraints(mp, pexprInferredPreds);
	}

	return pexprWithPreds;
}

//	Workhorse for pruning unused computed columns
//
//	The required columns passed by the query is passed to this pre-processing
//	stage and the list of columns are copied to a new list. This driver function
//	calls the PexprPruneUnusedComputedColsRecursive function with the copied
//	required column set. The original required columns set is not modified by
//	this preprocessor.
//
// 	Extra copy of the required columns set is avoided in each recursive call by
//	creating a one-time copy and passing it by reference for all the recursive
//	calls.
//
//	The functional behavior of the PruneUnusedComputedCols changed slightly
//	because we do not delete the required column set at the end of every
//	call but pass it to the next and consecutive recursive calls. However,
//	it is safe to add required columns by each operator we traverse, because non
//	of the required columns from other child of a tree will appear on the project
//	list of the other children.
//
// Therefore, the added columns to the required columns which is caused by
// the recursive call and passing by reference will not have a bad affect
// on the overall result.
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprPruneUnusedComputedCols(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefSet> pcrsReqd)
{
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == pcrsReqd ||
		GPOS_FTRACE(EopttraceDisablePruneUnusedComputedColumns))
	{
		return pexpr;
	}
	gpos::Ref<CColRefSet> pcrsReqdNew = GPOS_NEW(mp) CColRefSet(mp);
	pcrsReqdNew->Include(pcrsReqd);

	gpos::Ref<CExpression> pExprNew =
		PexprPruneUnusedComputedColsRecursive(mp, pexpr, pcrsReqdNew);

	return pExprNew;
}

// Workhorse for pruning unused computed columns
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprPruneUnusedComputedColsRecursive(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefSet> pcrsReqd)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();

	// leave subquery alone
	if (CUtils::FSubquery(pop))
	{
		return pexpr;
	}

	if (COperator::EopLogicalProject == pop->Eopid() ||
		COperator::EopLogicalGbAgg == pop->Eopid())
	{
		gpos::Ref<CExpression> pexprProjList = (*pexpr)[1];
		gpos::Ref<CColRefSet> pcrsDefined =
			pexprProjList->DeriveDefinedColumns();
		gpos::Ref<CColRefSet> pcrsSetReturningFunction =
			pexprProjList->DeriveSetReturningFunctionColumns();

		pcrsReqd->Include(CLogical::PopConvert(pop)->PcrsLocalUsed());
		// columns containing set-returning functions are needed for correct query results
		pcrsReqd->Union(pcrsSetReturningFunction);

		gpos::Ref<CColRefSet> pcrsUnusedLocal = GPOS_NEW(mp) CColRefSet(mp);
		pcrsUnusedLocal->Include(pcrsDefined);
		pcrsUnusedLocal->Difference(pcrsReqd);

		if (0 < pcrsUnusedLocal->Size())  // need to prune
		{
			// actual construction of new operators without unnecessary project elements
			gpos::Ref<CExpression> pexprResult =
				PexprPruneProjListProjectOrGbAgg(mp, pexpr, pcrsUnusedLocal,
												 pcrsDefined, pcrsReqd);

			return pexprResult;
		}
	}

	if (pop->FLogical())
	{
		// for logical operators, collect the used columns
		// this includes columns used by the operator itself and its scalar children
		CExpressionHandle exprhdl(mp);
		exprhdl.Attach(pexpr);
		gpos::Ref<CColRefSet> pcrsLogicalUsed = exprhdl.PcrsUsedColumns(mp);
		pcrsReqd->Include(pcrsLogicalUsed);
	}

	// process children
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG ulChildren = pexpr->Arity();

	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprPruneUnusedComputedColsRecursive(mp, (*pexpr)[ul], pcrsReqd);
		pdrgpexpr->Append(pexprChild);
	}



	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// Construct new Project or GroupBy operator without unused computed
// columns as project elements
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprPruneProjListProjectOrGbAgg(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefSet> pcrsUnused, gpos::Ref<CColRefSet> pcrsDefined,
	const gpos::Ref<CColRefSet> pcrsReqd)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pcrsUnused);
	GPOS_ASSERT(nullptr != pcrsDefined);
	GPOS_ASSERT(nullptr != pcrsReqd);

	gpos::Ref<CExpression> pexprResult = nullptr;
	gpos::Ref<COperator> pop = pexpr->Pop();
	gpos::Ref<CColRefSet> pcrsReqdNew = GPOS_NEW(mp) CColRefSet(mp);
	pcrsReqdNew->Include(pcrsReqd);

	GPOS_ASSERT(COperator::EopLogicalProject == pop->Eopid() ||
				COperator::EopLogicalGbAgg == pop->Eopid());

	gpos::Ref<CExpression> pexprRelational = (*pexpr)[0];
	gpos::Ref<CExpression> pexprProjList = (*pexpr)[1];

	// recursively process the relational child
	gpos::Ref<CExpression> pexprRelationalNew = nullptr;

	if (pcrsUnused->Size() == pcrsDefined->Size())
	{
		// the entire project list needs to be pruned
		if (COperator::EopLogicalProject == pop->Eopid())
		{
			pexprRelationalNew = PexprPruneUnusedComputedColsRecursive(
				mp, pexprRelational, pcrsReqdNew);
			pexprResult = pexprRelationalNew;
		}
		else
		{
			GPOS_ASSERT(COperator::EopLogicalGbAgg == pop->Eopid());

			gpos::Ref<CExpression> pexprProjectListNew = nullptr;
			gpos::Ref<CColRefArray> pdrgpcrGroupingCols =
				CLogicalGbAgg::PopConvert(pop)->Pdrgpcr();
			if (0 < pdrgpcrGroupingCols->Size())
			{
				// if grouping cols exist, we need to maintain the GbAgg with an empty project list
				pexprProjectListNew = GPOS_NEW(mp)
					CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp));
				pcrsReqdNew->Include(pdrgpcrGroupingCols);
			}
			else
			{
				// TODO:  10/15/2015: if there is no grouping cols, we could remove the entire GbAgg and plug in a ConstTableGet instead

				pexprProjectListNew = pexprProjList;
				CExpressionHandle exprhdl(mp);
				exprhdl.Attach(pexpr);
				gpos::Ref<CColRefSet> pcrsLogicalUsed =
					exprhdl.PcrsUsedColumns(mp);
				pcrsReqdNew->Include(pcrsLogicalUsed);
			}

			pexprRelationalNew = PexprPruneUnusedComputedColsRecursive(
				mp, pexprRelational, pcrsReqdNew);
			pexprResult = GPOS_NEW(mp)
				CExpression(mp, pop, pexprRelationalNew, pexprProjectListNew);
		}
	}
	else
	{
		// only remove part of the project elements
		gpos::Ref<CExpressionArray> pdrgpexprPrElRemain =
			GPOS_NEW(mp) CExpressionArray(mp);
		const ULONG ulPrjEls = pexprProjList->Arity();
		CExpressionHandle exprhdl(mp);

		for (ULONG ul = 0; ul < ulPrjEls; ul++)
		{
			gpos::Ref<CExpression> pexprPrEl = (*pexprProjList)[ul];
			gpos::Ref<CScalarProjectElement> popPrEl =
				CScalarProjectElement::PopConvert(pexprPrEl->Pop());
			if (!pcrsUnused->FMember(popPrEl->Pcr()))
			{
				pdrgpexprPrElRemain->Append(pexprPrEl);
				pcrsReqdNew->Include(pexprPrEl->DeriveUsedColumns());
			}
		}

		GPOS_ASSERT(0 < pdrgpexprPrElRemain->Size());
		gpos::Ref<CExpression> pexprNewProjectList = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprPrElRemain);

		pexprRelationalNew = PexprPruneUnusedComputedColsRecursive(
			mp, pexprRelational, pcrsReqdNew);
		pexprResult = GPOS_NEW(mp)
			CExpression(mp, pop, pexprRelationalNew, pexprNewProjectList);
	}


	return pexprResult;
}

// reorder the child for scalar comparision to ensure that left child is a scalar ident and right child is a scalar const if not
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprReorderScalarCmpChildren(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (CUtils::FScalarCmp(pexpr) ||
		COperator::EopScalarIsDistinctFrom == pexpr->Pop()->Eopid())
	{
		GPOS_ASSERT(2 == pexpr->Arity());
		gpos::Ref<CExpression> pexprLeft = (*pexpr)[0];
		gpos::Ref<CExpression> pexprRight = (*pexpr)[1];

		if (CUtils::FScalarConst(pexprLeft) && CUtils::FScalarIdent(pexprRight))
		{
			gpos::Ref<CScalarCmp> popScalarCmpCommuted =
				(gpos::dyn_cast<CScalarCmp>(pop))->PopCommutedOp(mp);
			if (popScalarCmpCommuted)
			{
				return GPOS_NEW(mp) CExpression(mp, popScalarCmpCommuted,
												pexprRight, pexprLeft);
			}
		}
	}

	// process children
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG ulChildren = pexpr->Arity();

	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprReorderScalarCmpChildren(mp, (*pexpr)[ul]);
		pdrgpexpr->Append(pexprChild);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

// converts IN subquery to a predicate AND an EXISTS subquery
// Example Algebrized queries:
// 1. Without a Project List:
//		Input:
//		+--CScalarSubqueryAny(=)["c2" (0)]
//		|--CLogicalGet "foo" ("foo"), Columns: ["c1" (8) ...] Key sets: {[1,7]}
//		+--CScalarIdent "c2" (0)
//
//		Output:
//		+--CScalarBoolOp (EboolopAnd)
//		|--CScalarCmp (=)
//		|  |--CScalarIdent "c2" (0)
//		|  +--CScalarIdent "c2" (0)
//		+--CScalarSubqueryExists
//		   +--CLogicalGet "foo" ("foo"), Columns: ["c1" (8) ...] Key sets: {[1,7]}
//
// 2. With a Project List:
//		Input:
//		+--CScalarSubqueryAny(=)["?column?" (16)]
//		|--CLogicalProject
//		|  |--CLogicalGet "foo" ("foo"), Columns: ["c1" (8) ...] Key sets: {[1,7]}
//		|  +--CScalarProjectList
//		|     +--CScalarProjectElement "?column?" (16)
//		|        +--CScalarOp (+)
//		|           |--CScalarIdent "c2" (0)
//		|           +--CScalarConst (1)
//		+--CScalarIdent "c2" (0)
//
//		Output:
//		+--CScalarBoolOp (EboolopAnd)
//		|--CScalarCmp (=)
//		|  |--CScalarIdent "c2" (0)
//		|  +--CScalarOp (+)
//		|     |--CScalarIdent "c2" (0)
//		|     +--CScalarConst (1)
//		+--CScalarSubqueryExists
//		   +--CLogicalGet "foo" ("foo"), Columns: ["c1" (8) ...] Key sets: {[1,7]}
gpos::Ref<CExpression>
CExpressionPreprocessor::ConvertInToSimpleExists(CMemoryPool *mp,
												 gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == pexpr->Pop()->Eopid());

	gpos::Ref<COperator> pop = pexpr->Pop();
	gpos::Ref<CExpression> pexprRelational = (*pexpr)[0];

	// Example for below variables:
	//   SELECT * FROM bar WHERE
	//       bar.a in (SELECT bar.b FROM foo)      <- Input expression (pexpr)
	//        |                |
	//    pexprLeft        pexprRight

	// generate scalarOp expression by using column reference of the IN subquery's
	// inner child's column reference as well as the expression extracted above
	// from the project element
	gpos::Ref<CExpression> pexprLeft = (*pexpr)[1];
	if (CUtils::FSubquery(pexprLeft->Pop()))
	{
		// don't convert if inner child is a subquery
		// Example: SELECT * FROM bar WHERE (SELECT 1) IN (SELECT c2 FROM foo);
		return nullptr;
	}

	// since Orca doesn't support IN subqueries of multiple columns such as
	// (a,a) in (select foo.a, foo.a from ...) ,
	// only extract the first expression under the first project element in the
	// project list and make it as the right operand to the scalar operation.
	gpos::Ref<CExpression> pexprRight = nullptr;
	gpos::Ref<CExpression> pexprSubqOfExists = nullptr;
	if (COperator::EopLogicalProject == pexprRelational->Pop()->Eopid())
	{
		pexprRight = CUtils::PNthProjectElementExpr(pexprRelational, 0);

		pexprSubqOfExists = (*pexprRelational)[0];
	}
	else
	{
		pexprRight = CUtils::PexprScalarIdent(
			mp, CScalarSubqueryAny::PopConvert(pop)->Pcr());
		pexprSubqOfExists = pexprRelational;
	}

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	gpos::Ref<IMDId> mdid = CScalarSubqueryAny::PopConvert(pop)->MdIdOp();
	const CWStringConst *str =
		md_accessor->RetrieveScOp(mdid)->Mdname().GetMDName();



	gpos::Ref<CExpression> pexprScalarOp =
		CUtils::PexprScalarCmp(mp, pexprLeft, pexprRight, *str, mdid);


	gpos::Ref<CExpression> pexprScalarSubqExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprSubqOfExists);

	// AND the generated predicate with the EXISTS subquery expression and return.
	gpos::Ref<CExpressionArray> pdrgpexprBoolOperands =
		GPOS_NEW(mp) CExpressionArray(mp);

	pdrgpexprBoolOperands->Append(pexprScalarOp);
	pdrgpexprBoolOperands->Append(pexprScalarSubqExists);

	return CUtils::PexprScalarBoolOp(mp, CScalarBoolOp::EboolopAnd,
									 pdrgpexprBoolOperands);
}

// rewrite IN subquery to EXIST subquery with a predicate
// Example:
//		Input:   SELECT * FROM foo WHERE foo.a IN (SELECT foo.b+1 FROM bar);
//		Output:  SELECT * FROM foo WHERE foo.a=foo.b+1 AND EXISTS (SELECT * FROM bar);
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprExistWithPredFromINSubq(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<COperator> pop = pexpr->Pop();

	// recursively process children
	const ULONG arity = pexpr->Arity();


	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			PexprExistWithPredFromINSubq(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	gpos::Ref<CExpression> pexprNew =
		GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
	//Check if the inner is a SubqueryAny
	if (CUtils::FAnySubquery(pop))
	{
		gpos::Ref<CExpression> pexprLogicalProject = (*pexprNew)[0];
		// we do the conversion if the project list has an outer reference and
		// it does not include any column from the relational child.
		if (COperator::EopLogicalProject == pexprLogicalProject->Pop()->Eopid())
		{
			// bail out if subquery has an inner reference or does not have any outer reference
			if (!CUtils::HasOuterRefs(pexprLogicalProject) ||
				CUtils::FInnerRefInProjectList(pexprLogicalProject))
			{
				return pexprNew;
			}
		}
		else
		{
			// perform conversion if subquery does not output any of the columns from relational child
			const gpos::Ref<CColRef> pcrSubquery =
				CScalarSubqueryAny::PopConvert(pop)->Pcr();
			gpos::Ref<CColRefSet> pcrsRelationalChild =
				(*pexpr)[0]->DeriveOutputColumns();
			if (pcrsRelationalChild->FMember(pcrSubquery))
			{
				return pexprNew;
			}
		}

		gpos::Ref<CExpression> pexprNewConverted =
			ConvertInToSimpleExists(mp, pexprNew);

		if (nullptr != pexprNewConverted)
		{
			pexprNew = pexprNewConverted;
			;
		}
	}

	return pexprNew;
}


gpos::Ref<CExpression>
CExpressionPreprocessor::PrunePartitions(CMemoryPool *mp,
										 gpos::Ref<CExpression> expr)
{
	GPOS_ASSERT(nullptr != expr);
	CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();

	gpos::Ref<COperator> pop = expr->Pop();
	if (pop->Eopid() == COperator::EopLogicalSelect &&
		(*expr)[0]->Pop()->Eopid() == COperator::EopLogicalDynamicGet)
	{
		gpos::Ref<CExpression> filter_pred = (*expr)[1];
		gpos::Ref<CLogicalDynamicGet> dyn_get =
			CLogicalDynamicGet::PopConvert((*expr)[0]->Pop());

		gpos::Ref<CColRefSetArray> pdrgpcrsChild = nullptr;
		gpos::Ref<CConstraint> pred_cnstr =
			CConstraint::PcnstrFromScalarExpr(mp, filter_pred, &pdrgpcrsChild);


		gpos::Ref<IMdIdArray> selected_partition_mdids =
			GPOS_NEW(mp) IMdIdArray(mp);
		gpos::Ref<CConstraintArray> selected_partition_cnstrs =
			GPOS_NEW(mp) CConstraintArray(mp);

		gpos::Ref<IMdIdArray> foreign_server_mdids =
			GPOS_NEW(mp) IMdIdArray(mp);
		gpos::Ref<IMdIdArray> all_partition_mdids =
			dyn_get->GetPartitionMdids();
		for (ULONG ul = 0; ul < all_partition_mdids->Size(); ++ul)
		{
			gpos::Ref<IMDId> part_mdid = (*all_partition_mdids)[ul];
			const gpos::Ref<IMDRelation> partrel = mda->RetrieveRel(part_mdid);

			gpos::Ref<CConstraint> rel_cnstr = PcnstrFromChildPartition(
				partrel, dyn_get->PdrgpcrOutput(),
				(*dyn_get->GetRootColMappingPerPart())[ul]);

			gpos::Ref<CConstraint> pcnstr = nullptr;
			{
				gpos::Ref<CConstraintArray> preds =
					GPOS_NEW(mp) CConstraintArray(mp);
				if (pred_cnstr != nullptr)
				{
					preds->Append(pred_cnstr);
				}
				if (rel_cnstr != nullptr)
				{
					preds->Append(rel_cnstr);
				}
				pcnstr = CConstraint::PcnstrConjunction(mp, preds);
			}

			// Include the partition if it's not a contradiction, or if it's
			// undefined (e.g only has default partition)
			if (nullptr == pcnstr || !pcnstr->FContradiction())
			{
				gpos::Ref<IMDId> foreign_server_mdid =
					(*dyn_get->ForeignServerMdIds())[ul];

				foreign_server_mdids->Append(foreign_server_mdid);

				selected_partition_mdids->Append(part_mdid);
				rel_cnstr = PcnstrFromChildPartition(
					partrel, dyn_get->PdrgpcrOutput(),
					(*dyn_get->GetRootColMappingPerPart())[ul]);
				if (rel_cnstr)
				{
					selected_partition_cnstrs->Append(rel_cnstr);
				}
			}
		}


		if (selected_partition_mdids->Size() == 0)
		{
			// Return const false if there are no partitions left to scan



			gpos::Ref<CColRefArray> colref_array =
				expr->DeriveOutputColumns()->Pdrgpcr(mp);

			gpos::Ref<IDatum2dArray> pdrgpdrgpdatum =
				GPOS_NEW(mp) IDatum2dArray(mp);
			gpos::Ref<COperator> popCTG = GPOS_NEW(mp)
				CLogicalConstTableGet(mp, colref_array, pdrgpdrgpdatum);
			return GPOS_NEW(mp) CExpression(mp, popCTG);
		}

		CName *new_alias = GPOS_NEW(mp) CName(mp, dyn_get->Name());



		gpos::Ref<CConstraint> selected_part_cnstr_disj =
			CConstraint::PcnstrDisjunction(mp, selected_partition_cnstrs);

		gpos::Ref<CLogicalDynamicGet> new_dyn_get =
			GPOS_NEW(mp) CLogicalDynamicGet(
				mp, new_alias, dyn_get->Ptabdesc(), dyn_get->ScanId(),
				dyn_get->PdrgpcrOutput(), dyn_get->PdrgpdrgpcrPart(),
				selected_partition_mdids, selected_part_cnstr_disj, true,
				foreign_server_mdids);

		gpos::Ref<CExpressionArray> select_children =
			GPOS_NEW(mp) CExpressionArray(mp);
		select_children->Append(GPOS_NEW(mp) CExpression(mp, new_dyn_get));
		select_children->Append(PrunePartitions(mp, filter_pred));


		return GPOS_NEW(mp) CExpression(mp, pop, select_children);
	}

	// process children
	gpos::Ref<CExpressionArray> children = GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < expr->Arity(); ul++)
	{
		gpos::Ref<CExpression> expr_child = (*expr)[ul];
		gpos::Ref<CExpression> new_expr_child = PrunePartitions(mp, expr_child);
		children->Append(new_expr_child);
	}


	return GPOS_NEW(mp) CExpression(mp, pop, children);
}

// Translate the part constraint of a child partition into an ORCA expr using
// corresponding colrefs of the root table, instead of those from the child
// partition.
gpos::Ref<CConstraint>
CExpressionPreprocessor::PcnstrFromChildPartition(
	const gpos::Ref<IMDRelation> partrel, gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<ColRefToUlongMap> root_col_mapping)
{
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	CMemoryPool *mp = COptCtxt::PoctxtFromTLS()->Pmp();

	gpos::Ref<CExpression> part_constraint_expr = nullptr;

	gpos::Ref<CDXLNode> dxlnode = partrel->MDPartConstraint();

	if (nullptr == dxlnode)
	{
		return nullptr;
	}

	// Create a list of indexes into the columns of the table descriptor of the
	// child partition. This is used in PexprTranslateScalar() to construct a
	// reverse mapping from the colid of  each (non-dropped) column in the child
	// partition to it's corresponding colref in the root table.
	// NB: For the indexes in root_col_mapping to be applied correctly here, the
	// part constraint should have been translated without dropped cols
	// (see RetrievePartConstraintForRel()).
	gpos::Ref<ULongPtrArray> mapped_colids = GPOS_NEW(mp) ULongPtrArray(mp);
	for (ULONG ul = 0; ul < pdrgpcrOutput->Size(); ++ul)
	{
		gpos::Ref<CColRef> colref = (*pdrgpcrOutput)[ul];
		gpos::Ref<UlongRef> colid = root_col_mapping->Find(colref);
		GPOS_ASSERT(nullptr != colid);
		mapped_colids->Append(GPOS_NEW(mp) ULONG(colid->value()));
	}

	CTranslatorDXLToExpr dxltr(mp, md_accessor);
	part_constraint_expr =
		dxltr.PexprTranslateScalar(dxlnode, pdrgpcrOutput, mapped_colids);


	GPOS_ASSERT(CUtils::FPredicate(part_constraint_expr));

	gpos::Ref<CColRefSetArray> pdrgpcrsChild = nullptr;
	gpos::Ref<CConstraint> cnstr = CConstraint::PcnstrFromScalarExpr(
		mp, part_constraint_expr, &pdrgpcrsChild, true /* infer_nulls_as */);



	GPOS_ASSERT(cnstr);
	return cnstr;
}

// Collapse a select over a project and update column reference.
gpos::Ref<CExpression>
CExpressionPreprocessor::CollapseSelectAndReplaceColref(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr, gpos::Ref<CColRef> pcolref,
	gpos::Ref<CExpression> pprojExpr)
{
	// remove the logical project
	//
	// Input:
	// +--CLogicalSelect (x = 'meh')
	//    +--CLogicalProject (col1...n, expr as x)
	//       +-- CLogicalNAryJoin
	// Output:
	// +--CLogicalSelect (expr = 'meh')
	//    +-- CLogicalNAryJoin
	if (pexpr->Pop()->Eopid() == COperator::EopLogicalSelect &&
		(*pexpr)[0]->Pop()->Eopid() == COperator::EopLogicalProject &&
		(*(*pexpr)[0])[0]->Pop()->Eopid() == COperator::EopLogicalNAryJoin)
	{
		gpos::Ref<CExpression> pexprCollapsedSelect = GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), (*(*pexpr)[0])[0],
						CollapseSelectAndReplaceColref(mp, (*pexpr)[1], pcolref,
													   pprojExpr));

		gpos::Ref<CExpression> pexprTransposed =
			PexprTransposeSelectAndProject(mp, pexprCollapsedSelect);

		return pexprTransposed;
	}

	// replace reference
	if (pexpr->Pop()->Eopid() == COperator::EopScalarIdent &&
		CColRef::Equals(CScalarIdent::PopConvert(pexpr->Pop())->Pcr(), pcolref))
	{
		return pprojExpr;
	}

	// recurse to children
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		pdrgpexprChildren->Append(CollapseSelectAndReplaceColref(
			mp, (*pexpr)[ul], pcolref, pprojExpr));
	}

	gpos::Ref<COperator> pop = pexpr->Pop();

	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
}

// Transpose a select over a project
//
// This preprocessing step enables additional opportunities for predicate push
// down.  In the following example, the select constraint references "b" (18)
// from the project list. As-is the normalizer cannot push the contraint further
// than the project.  This step enables further push down by transposing the
// project/select operators and fixing up corresponding column references.
//
// Example:
//
// Input:
// +--CLogicalSelect
//    |--CLogicalProject
//    |  |--CLogicalNAryJoin
//    |  |  |--CLogicalGet "foo" ("foo"), Columns: ["a" (0), "b" (1), ...
//    |  |  |--CLogicalGet "bar" ("bar"), Columns: ["c" (9), "d" (10), ...
//    |  |  +--CScalarCmp (=)
//    |  |     |--CScalarIdent "a" (0)
//    |  |     +--CScalarIdent "c" (9)
//    |  +--CScalarProjectList
//    |     +--CScalarProjectElement "b" (18)
//    |        +--CScalarFunc (varchar)
//    |           |--CScalarCast
//    |           |  +--CScalarIdent "b" (1)
//    |           |--CScalarConst (104)
//    |           +--CScalarConst (1)
//    +--CScalarCmp (=)
//       |--CScalarCast
//       |  +--CScalarIdent "b" (18)
//       +--CScalarConst (1828233457.000)
// Output:
// +--CLogicalProject
//    |--CLogicalSelect
//    |  |--CLogicalNAryJoin
//    |  |  |--CLogicalGet "foo" ("foo"), Columns: ["a" (0), "b" (1), ...
//    |  |  |--CLogicalGet "bar" ("bar"), Columns: ["c" (9), "d" (10), ...
//    |  |  +--CScalarCmp (=)
//    |  |     |--CScalarIdent "a" (0)
//    |  |     +--CScalarIdent "c" (9)
//    |  +--CScalarCmp (=)
//    |     |--CScalarCast
//    |     |  +--CScalarFunc (varchar)
//    |     |     |--CScalarCast
//    |     |     |  +--CScalarIdent "b" (1)
//    |     |     |--CScalarConst (104)
//    |     |     +--CScalarConst (1)
//    |     +--CScalarConst (1828233457.000)
//    +--CScalarProjectList
//       +--CScalarProjectElement "b" (18)
//          +--CScalarFunc (varchar)
//             |--CScalarCast
//             |  +--CScalarIdent "b" (1)
//             |--CScalarConst (104)
//             +--CScalarConst (1)
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprTransposeSelectAndProject(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	// Transpose:
	//
	// Input:
	// +--CLogicalSelect (x = 'meh')
	//    +--CLogicalProject (col1...n, expr as x)
	//       +-- CLogicalNAryJoin
	// Output:
	// +--CLogicalProject (col1..n, expr as x)
	//    +--CLogicalSelect (expr = 'meh')
	//       +-- CLogicalNAryJoin
	if (pexpr->Pop()->Eopid() == COperator::EopLogicalSelect &&
		(*pexpr)[0]->Pop()->Eopid() == COperator::EopLogicalProject &&
		(*(*pexpr)[0])[0]->Pop()->Eopid() == COperator::EopLogicalNAryJoin)
	{
		gpos::Ref<CExpression> pproject = (*pexpr)[0];
		gpos::Ref<CExpression> pprojectList = (*pproject)[1];
		gpos::Ref<CExpression> pselectNew = pexpr;

		gpos::Ref<CExpressionArray> pdrgpexpr =
			GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ul = 0; ul < pprojectList->Arity(); ul++)
		{
			gpos::Ref<CExpression> pprojexpr =
				CUtils::PNthProjectElementExpr(pproject, ul);

			if (pprojexpr->DeriveHasNonScalarFunction() ||
				pprojexpr->DeriveHasSubquery())
			{
				// Bail if project expression contains a set-returning function
				// or subquery


				return pexpr;
			}

			CExpressionHandle exprhdl(mp);
			exprhdl.Attach(pprojexpr);
			exprhdl.DeriveProps(nullptr /*pdpctxt*/);

			if (exprhdl.FChildrenHaveVolatileFunc())
			{
				// Bail if project expression contains a volatile function


				return pexpr;
			}

			// TODO: In order to support mixed pushable and non-pushable
			//       predicates we need to be able to deconstruct a select
			//       conjunction constraint into pushable and non-pushable
			//       parts.
			//
			//       NB: JoinOnViewWithMixOfPushableAndNonpushablePredicates.mdp
			gpos::Ref<CExpression> prevpselectNew = pselectNew;
			pselectNew = CollapseSelectAndReplaceColref(
				mp, prevpselectNew,
				CUtils::PNthProjectElement(pproject, ul)->Pcr(),
				CUtils::PNthProjectElementExpr(pproject, ul));
			if (pexpr != prevpselectNew)
			{
			}
		}
		pdrgpexpr->Append(pselectNew);

		gpos::Ref<CExpressionArray> pdrgpprojelems =
			GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ul = 0; ul < pprojectList->Arity(); ul++)
		{
			pdrgpprojelems->Append((*pprojectList)[ul]);
		}
		pdrgpexpr->Append(GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpprojelems));

		return GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), pdrgpexpr);
	}
	else
	{
		gpos::Ref<CExpressionArray> pdrgpexprChildren =
			GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
		{
			pdrgpexprChildren->Append(
				PexprTransposeSelectAndProject(mp, (*pexpr)[ul]));
		}

		gpos::Ref<COperator> pop = pexpr->Pop();

		return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexprChildren);
	}
}

// Preprocessor function to decide if the Update operator to be proceeded with
// split update or inplace update based on the columns modified by the update
// operation.

// Split Update if any of the modified columns is a distribution column or a partition key,
// InPlace Update if all the modified columns are non-distribution columns and not partition keys.

// Example: Update Modified non-distribution columns.
// Input:
// +--CLogicalUpdate ("foo"), Split Update, Delete Columns: ["a" (0), "b" (1)], Insert Columns: ["a" (0), "b" (9)], "ctid" (2), "gp_segment_id" (8)
//   +--CLogicalProject
//      |--CLogicalGet
//      +--CScalarProjectList
//
// Output:
// +--CLogicalUpdate ("foo"), In-place Update, Delete Columns: ["a" (0), "b" (1)], Insert Columns: ["a" (0), "b" (9)], "ctid" (2), "gp_segment_id" (8)
//	 +--CLogicalProject
//		|--CLogicalGet
//		+--CScalarProjectList

gpos::Ref<CExpression>
CExpressionPreprocessor::ConvertSplitUpdateToInPlaceUpdate(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);
	gpos::Ref<COperator> pop = pexpr->Pop();
	if (COperator::EopLogicalUpdate != pop->Eopid())
	{
		return pexpr;
	}
	gpos::Ref<CLogicalUpdate> popUpdate = CLogicalUpdate::PopConvert(pop);
	gpos::Ref<CTableDescriptor> tabdesc = popUpdate->Ptabdesc();
	gpos::Ref<CColRefArray> pdrgpcrInsert = popUpdate->PdrgpcrInsert();
	gpos::Ref<CColRefArray> pdrgpcrDelete = popUpdate->PdrgpcrDelete();
	const ULONG num_cols = pdrgpcrInsert->Size();
	BOOL split_update = false;
	gpos::Ref<CColRefArray> ppartColRefs = GPOS_NEW(mp) CColRefArray(mp);
	const gpos::Ref<ULongPtrArray> pdrgpulPart = tabdesc->PdrgpulPart();
	const ULONG ulPartKeys = pdrgpulPart->Size();

	// Uses split update if any of the modified columns are either
	// distribution or partition keys.
	// FIXME: Tested on distribution columns. Validate this after DML on
	//  partitioned tables is implemented in Orca
	for (ULONG ul = 0; ul < ulPartKeys; ul++)
	{
		ULONG *pulPartKey = (*pdrgpulPart)[ul];
		gpos::Ref<CColRef> colref = (*pdrgpcrInsert)[*pulPartKey];
		ppartColRefs->Append(colref);
	}

	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		gpos::Ref<CColRef> pcrInsert = (*pdrgpcrInsert)[ul];
		gpos::Ref<CColRef> pcrDelete = (*pdrgpcrDelete)[ul];
		// Checking if column is either distribution or partition key.
		if (pcrInsert != pcrDelete &&
			(pcrDelete->IsDistCol() ||
			 std::find(ppartColRefs->begin(), ppartColRefs->end(), pcrInsert) !=
				 ppartColRefs->end()))
		{
			split_update = true;
			break;
		}
	}

	if (!split_update)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[0];



		gpos::Ref<CExpression> pexprNew = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CLogicalUpdate(mp, tabdesc, pdrgpcrDelete,
										pdrgpcrInsert, popUpdate->PcrCtid(),
										popUpdate->PcrSegmentId(), false),
			pexprChild);
		return pexprNew;
	}

	return pexpr;
}

// main driver, pre-processing of input logical expression
gpos::Ref<CExpression>
CExpressionPreprocessor::PexprPreprocess(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefSet>
		pcrsOutputAndOrderCols	// query output cols and cols used in the order specs
)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	CAutoTimer at("\n[OPT]: Expression Preprocessing Time",
				  GPOS_FTRACE(EopttracePrintOptimizationStatistics));

	// (1) remove unused CTE anchors
	gpos::Ref<CExpression> pexprNoUnusedCTEs = PexprRemoveUnusedCTEs(mp, pexpr);
	GPOS_CHECK_ABORT;

	// (2.a) remove intermediate superfluous limit
	gpos::Ref<CExpression> pexprSimplifiedLimit =
		PexprRemoveSuperfluousLimit(mp, pexprNoUnusedCTEs);
	GPOS_CHECK_ABORT;


	// (2.b) remove intermediate superfluous distinct
	gpos::Ref<CExpression> pexprSimplifiedDistinct =
		PexprRemoveSuperfluousDistinctInDQA(mp, pexprSimplifiedLimit);
	GPOS_CHECK_ABORT;


	// (3) trim unnecessary existential subqueries
	gpos::Ref<CExpression> pexprTrimmed =
		PexprTrimExistentialSubqueries(mp, pexprSimplifiedDistinct);
	GPOS_CHECK_ABORT;


	// (4) collapse cascaded union / union all
	gpos::Ref<CExpression> pexprNaryUnionUnionAll =
		PexprCollapseUnionUnionAll(mp, pexprTrimmed);
	GPOS_CHECK_ABORT;


	// (5) remove superfluous outer references from the order spec in limits, grouping columns in GbAgg, and
	// Partition/Order columns in window operators
	gpos::Ref<CExpression> pexprOuterRefsEleminated =
		PexprRemoveSuperfluousOuterRefs(mp, pexprNaryUnionUnionAll);
	GPOS_CHECK_ABORT;


	// (6) remove superfluous equality
	gpos::Ref<CExpression> pexprTrimmed2 =
		PexprPruneSuperfluousEquality(mp, pexprOuterRefsEleminated);
	GPOS_CHECK_ABORT;


	// (7) substitute constant predicates
	gpos::Ref<ExprToConstantMap> phmExprToConst =
		GPOS_NEW(mp) ExprToConstantMap(mp);
	gpos::Ref<CExpression> pexprPredWithConstReplaced =
		PexprReplaceColWithConst(mp, pexprTrimmed2, phmExprToConst, true);
	GPOS_CHECK_ABORT;



	// (8) simplify quantified subqueries
	gpos::Ref<CExpression> pexprSubqSimplified =
		PexprSimplifyQuantifiedSubqueries(mp, pexprPredWithConstReplaced);
	GPOS_CHECK_ABORT;


	// (9) do preliminary unnesting of scalar subqueries
	gpos::Ref<CExpression> pexprSubqUnnested =
		PexprUnnestScalarSubqueries(mp, pexprSubqSimplified);
	GPOS_CHECK_ABORT;


	// (10) unnest AND/OR/NOT predicates
	gpos::Ref<CExpression> pexprUnnested =
		CExpressionUtils::PexprUnnest(mp, pexprSubqUnnested);
	GPOS_CHECK_ABORT;


	gpos::Ref<CExpression> pexprConvert2In = pexprUnnested;

	// GPDB_12_MERGE_FIXME: Although we've enabled EopttraceArrayConstraints,
	// the following conversion is causing problems; and might be very
	// inefficient! Disable for noe.
	if (GPOS_FTRACE(EopttraceArrayConstraints) && false)
	{
		// (10.5) ensure predicates are array IN or NOT IN where applicable
		pexprConvert2In = PexprConvert2In(mp, pexprUnnested);
		GPOS_CHECK_ABORT;
	}

	// (11) infer predicates from constraints
	gpos::Ref<CExpression> pexprInferredPreds =
		PexprInferPredicates(mp, pexprConvert2In);
	GPOS_CHECK_ABORT;


	// (12) eliminate self comparisons
	gpos::Ref<CExpression> pexprSelfCompEliminated =
		PexprEliminateSelfComparison(mp, pexprInferredPreds);
	GPOS_CHECK_ABORT;


	// (13) remove duplicate AND/OR children
	gpos::Ref<CExpression> pexprDeduped =
		CExpressionUtils::PexprDedupChildren(mp, pexprSelfCompEliminated);
	GPOS_CHECK_ABORT;


	// (14) factorize common expressions
	gpos::Ref<CExpression> pexprFactorized =
		CExpressionFactorizer::PexprFactorize(mp, pexprDeduped);
	GPOS_CHECK_ABORT;


	// (15) infer filters out of components of disjunctive filters
	gpos::Ref<CExpression> pexprPrefiltersExtracted =
		CExpressionFactorizer::PexprExtractInferredFilters(mp, pexprFactorized);
	GPOS_CHECK_ABORT;


	// (16) pre-process ordered agg functions
	gpos::Ref<CExpression> pexprOrderedAggPreprocessed =
		COrderedAggPreprocessor::PexprPreprocess(mp, pexprPrefiltersExtracted);
	GPOS_CHECK_ABORT;


	// (17) pre-process window functions
	gpos::Ref<CExpression> pexprWindowPreprocessed =
		CWindowPreprocessor::PexprPreprocess(mp, pexprOrderedAggPreprocessed);
	GPOS_CHECK_ABORT;


	// (18) eliminate unused computed columns
	gpos::Ref<CExpression> pexprNoUnusedPrEl = PexprPruneUnusedComputedCols(
		mp, pexprWindowPreprocessed, pcrsOutputAndOrderCols);
	GPOS_CHECK_ABORT;


	// (19) normalize expression
	gpos::Ref<CExpression> pexprNormalized1 =
		CNormalizer::PexprNormalize(mp, pexprNoUnusedPrEl);
	GPOS_CHECK_ABORT;


	// (20) transform outer join into inner join whenever possible
	gpos::Ref<CExpression> pexprLOJToIJ =
		PexprOuterJoinToInnerJoin(mp, pexprNormalized1);
	GPOS_CHECK_ABORT;


	// (21) collapse cascaded inner and left outer joins
	gpos::Ref<CExpression> pexprCollapsed =
		PexprCollapseJoins(mp, pexprLOJToIJ);
	GPOS_CHECK_ABORT;


	// (22) after transforming outer joins to inner joins, we may be able to generate more predicates from constraints
	gpos::Ref<CExpression> pexprWithPreds =
		PexprAddPredicatesFromConstraints(mp, pexprCollapsed);
	GPOS_CHECK_ABORT;


	// (23) eliminate empty subtrees
	gpos::Ref<CExpression> pexprPruned =
		PexprPruneEmptySubtrees(mp, pexprWithPreds);
	GPOS_CHECK_ABORT;


	// (24) collapse cascade of projects
	gpos::Ref<CExpression> pexprCollapsedProjects =
		PexprCollapseProjects(mp, pexprPruned);
	GPOS_CHECK_ABORT;


	// (25) insert dummy project when the scalar subquery is under a project and returns an outer reference
	gpos::Ref<CExpression> pexprSubquery = PexprProjBelowSubquery(
		mp, pexprCollapsedProjects, false /* fUnderPrList */);
	GPOS_CHECK_ABORT;


	// (26) reorder the children of scalar cmp operator to ensure that left child is scalar ident and right child is scalar const
	gpos::Ref<CExpression> pexrReorderedScalarCmpChildren =
		PexprReorderScalarCmpChildren(mp, pexprSubquery);
	GPOS_CHECK_ABORT;


	// (27) rewrite IN subquery to EXIST subquery with a predicate
	gpos::Ref<CExpression> pexprExistWithPredFromINSubq =
		PexprExistWithPredFromINSubq(mp, pexrReorderedScalarCmpChildren);
	GPOS_CHECK_ABORT;


	// (28) prune partitions
	gpos::Ref<CExpression> pexprPrunedPartitions =
		PrunePartitions(mp, pexprExistWithPredFromINSubq);
	GPOS_CHECK_ABORT;


	// (29) swap logical select over logical project
	gpos::Ref<CExpression> pexprTransposeSelectAndProject =
		PexprTransposeSelectAndProject(mp, pexprPrunedPartitions);


	// (30) convert split update to inplace update
	gpos::Ref<CExpression> pexprSplitUpdateToInplace =
		ConvertSplitUpdateToInPlaceUpdate(mp, pexprTransposeSelectAndProject);
	GPOS_CHECK_ABORT;


	// (31) normalize expression again
	gpos::Ref<CExpression> pexprNormalized2 =
		CNormalizer::PexprNormalize(mp, pexprSplitUpdateToInplace);
	GPOS_CHECK_ABORT;


	return pexprNormalized2;
}

// EOF
