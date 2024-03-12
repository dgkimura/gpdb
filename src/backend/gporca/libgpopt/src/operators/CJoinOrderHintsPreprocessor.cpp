//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2024 VMware, Inc. or its affiliates.
//
//	@filename:
//		CJoinOrderHintsPreprocessor.cpp
//
//	@doc:
//		Preprocessing routines of join order hints
//---------------------------------------------------------------------------
#include "gpopt/operators/CJoinOrderHintsPreprocessor.h"

#include <limits>

#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CLogicalRightOuterJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarNAryJoinPredList.h"
#include "gpopt/optimizer/COptimizerConfig.h"

using namespace gpopt;

constexpr ULONG MAX_INDEX = std::numeric_limits<ULONG>::max();


//---------------------------------------------------------------------------
//	@function:
//		GetChildWithSingleAlias
//
//	@doc:
//		Return the child expression of pexpr that has a single table descriptor
//		matching alias. If none exists, return nullptr.
//---------------------------------------------------------------------------
static CExpression *
GetChildWithSingleAlias(CExpression *pexpr, const CWStringConst *alias,
						ULONG *index)
{
	GPOS_ASSERT(COperator::EopLogicalNAryJoin == pexpr->Pop()->Eopid());

	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CTableDescriptorHashSet *ptabs = (*pexpr)[ul]->DeriveTableDescriptor();
		if (ptabs->Size() == 1 && ptabs->First()->Name().Pstr()->Equals(alias))
		{
			if (nullptr != index &&
				COperator::EopScalarNAryJoinPredList ==
					(*pexpr)[pexpr->Arity() - 1]->Pop()->Eopid())
			{
				if (nullptr != (*CLogicalNAryJoin::PopConvert(pexpr->Pop())
									 ->GetLojChildPredIndexes())[ul])
				{
					*index = *(*CLogicalNAryJoin::PopConvert(pexpr->Pop())
									->GetLojChildPredIndexes())[ul];
				}
			}
			return (*pexpr)[ul];
		}
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		GetChild
//
//	@doc:
//		Return the child expression of pexpr that has a table descriptor set
//		containing the aliases. If none exists, return nullptr.
//---------------------------------------------------------------------------
static CExpression *
GetChild(CMemoryPool *mp, CExpression *pexpr, StringPtrArray *aliases,
		 ULONG *index = nullptr)
{
	GPOS_ASSERT(COperator::EopLogicalNAryJoin == pexpr->Pop()->Eopid());

	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		StringPtrArray *pexpr_aliases =
			CHintUtils::GetAliasesFromTableDescriptors(
				mp, (*pexpr)[ul]->DeriveTableDescriptor());

		bool is_contained = true;

		for (ULONG j = 0; j < aliases->Size(); j++)
		{
			if (nullptr == pexpr_aliases->Find((*aliases)[j]))
			{
				is_contained = false;
				break;
			}
		}

		pexpr_aliases->Release();
		if (is_contained)
		{
			if (nullptr != index &&
				COperator::EopScalarNAryJoinPredList ==
					(*pexpr)[pexpr->Arity() - 1]->Pop()->Eopid())
			{
				if (nullptr != (*CLogicalNAryJoin::PopConvert(pexpr->Pop())
									 ->GetLojChildPredIndexes())[ul])
				{
					*index = *(*CLogicalNAryJoin::PopConvert(pexpr->Pop())
									->GetLojChildPredIndexes())[ul];
				}
			}
			return (*pexpr)[ul];
		}
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		IsChild
//
//	@doc:
//		Return whether a child expression of pexpr that has a table descriptor
//		set containing the aliases.
//---------------------------------------------------------------------------
static bool
IsChild(CMemoryPool *mp, CExpression *pexpr, StringPtrArray *aliases)
{
	return nullptr != GetChild(mp, pexpr, aliases);
}


//---------------------------------------------------------------------------
//	@function:
//		IsAliasSubsetOrDisjoint
//
//	@doc:
//		Return whether a table descriptor set contains all or none of the
//		aliases.
//---------------------------------------------------------------------------
GPOS_ASSERTS_ONLY static bool
IsAliasSubsetOrDisjoint(CTableDescriptorHashSet *tabdescs,
						StringPtrArray *aliases)
{
	int includedCount = 0;
	int excludedCount = 0;
	CTableDescriptorHashSetIter hsiter(tabdescs);
	while (hsiter.Advance())
	{
		CTableDescriptor *tabdesc =
			const_cast<CTableDescriptor *>(hsiter.Get());
		if (nullptr == aliases->Find(tabdesc->Name().Pstr()))
		{
			excludedCount += 1;
		}
		else
		{
			includedCount += 1;
		}

		if (excludedCount > 0 && includedCount > 0)
		{
			return false;
		}
	}
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CJoinOrderHintsPreprocessor::ConstructNAryJoinChildren
//
//	@doc:
//		Return list of children for the new NAry join expression.
//
//		"naryJoinPexpr" is the original NAry join and "binaryJoinExpr" is the
//		processed version with hints applied. Some children from the original
//		NAry join "naryJoinPexpr" may not be included in "binaryJoinExpr"
//		because they were not used in the hints. Those children need to be
//		added.
//---------------------------------------------------------------------------
CExpressionArray *
CJoinOrderHintsPreprocessor::ConstructNAryJoinChildren(
	CMemoryPool *mp, CExpression *naryJoinPexpr, CExpression *binaryJoinExpr,
	std::set<ULONG> &usedChildrenIndexes,
	GPOS_UNUSED std::set<ULONG> &usedPredicateIndexes)
{
	GPOS_ASSERT(COperator::EopLogicalNAryJoin == naryJoinPexpr->Pop()->Eopid());

	// get all the alias used in the binaryJoinExpr
	StringPtrArray *usedNames = CHintUtils::GetAliasesFromTableDescriptors(
		mp, binaryJoinExpr->DeriveTableDescriptor());

	CExpressionArray *naryChildren = GPOS_NEW(mp) CExpressionArray(mp);
	if (usedChildrenIndexes.count(0) > 0)
	{
		naryChildren->Append(binaryJoinExpr);
	}

	// check for hint aliases not used in any child
	for (ULONG ul = 0; ul < naryJoinPexpr->Arity(); ul++)
	{
		if ((*naryJoinPexpr)[ul]->DeriveTableDescriptor()->Size() == 0)
		{
			continue;
		}

		GPOS_ASSERT(IsAliasSubsetOrDisjoint(
			(*naryJoinPexpr)[ul]->DeriveTableDescriptor(), usedNames));

		const CWStringConst *alias = (*naryJoinPexpr)[ul]
										 ->DeriveTableDescriptor()
										 ->First()
										 ->Name()
										 .Pstr();
		if (nullptr == usedNames->Find(alias))
		{
			// If the child expression has an alias that isn't in the
			// "usedNames" set , then the child expression hasn't been
			// processed yet. Process it now.
			naryChildren->Append(CJoinOrderHintsPreprocessor::PexprPreprocess(
				mp, (*naryJoinPexpr)[ul]));
		}
	}
	if (usedChildrenIndexes.count(0) == 0)
	{
		naryChildren->Append(binaryJoinExpr);
	}

	usedNames->Release();
	return naryChildren;
}


static bool
IsValidLOJPred(CMemoryPool *mp, CExpression *outer, CExpression *inner,
			   CExpression *pred)
{
	CColRefSet *innerAndOuter = GPOS_NEW(mp) CColRefSet(mp);
	innerAndOuter->Include(outer->DeriveOutputColumns());
	innerAndOuter->Include(inner->DeriveOutputColumns());

	CColRefSet *predCols = pred->DeriveUsedColumns();

	bool result = innerAndOuter->ContainsAll(predCols);
	innerAndOuter->Release();
	return result;
}


//---------------------------------------------------------------------------
//	@function:
//		GetOnPreds
//
//	@doc:
//		Builds the predicates on inner join expressions.
//---------------------------------------------------------------------------
static CExpression *
GetOnPreds(CMemoryPool *mp, CExpression *outer, CExpression *inner,
		   CExpressionArray *allOnPreds)
{
	CExpressionArray *preds = GPOS_NEW(mp) CExpressionArray(mp);

	CColRefSet *innerAndOuter = GPOS_NEW(mp) CColRefSet(mp);
	innerAndOuter->Include(outer->DeriveOutputColumns());
	innerAndOuter->Include(inner->DeriveOutputColumns());

	for (ULONG ul = 0; ul < allOnPreds->Size(); ul++)
	{
		CColRefSet *predCols = (*allOnPreds)[ul]->DeriveUsedColumns();
		GPOS_ASSERT(predCols != nullptr);

		if (innerAndOuter->ContainsAll(predCols) &&
			predCols->FIntersects(outer->DeriveOutputColumns()) &&
			predCols->FIntersects(inner->DeriveOutputColumns()))
		{
			// if both inner and outer contain cover the predicate columns then
			// add the predicate.
			(*allOnPreds)[ul]->AddRef();
			preds->Append((*allOnPreds)[ul]);
		}
	}

	if (preds->Size() > 0)
	{
		innerAndOuter->Release();
		// create a conjunction of all the found predicates.
		return CPredicateUtils::PexprConjunction(mp, preds);
	}

	preds->Release();
	innerAndOuter->Release();

	// if no explicit predicate found, use "ON true".
	return CUtils::PexprScalarConstBool(mp, /*fVal*/ true, /*is_null*/ false);
}


//---------------------------------------------------------------------------
//	@function:
//		CJoinOrderHintsPreprocessor::RecursiveApplyJoinOrderHintsOnNAryJoin
//
//	@doc:
//		Recursively apply a hint on a Nary join expression to bottom-up build
//		a new expression using Inner and Left/Right Outer join operators.
//
//		For example, let's say the expression tree is:
//
//		  +--CLogicalNAryJoin
//		     |--CLogicalGet "t1" ("t1"), Columns: ["a" (0), "b" (1),...
//		     |--CLogicalLimit <empty> global
//		     |  |--CLogicalNAryJoin
//		     |  |  |--CLogicalGet "t2" ("t2"), Columns: ["a" (9), "b" (10),...
//		     |  |  |--CLogicalGet "t3" ("t3"), Columns: ["a" (18), "b" (19),...
//		     |  |  ...
//		     |  ...
//		     ...
//
//		And the hint is Leading((t1 (t2 t3))).
//
//		Our first step is to separate the left and rigt side of the hint:
//		  left-side: "t1"
//		  right-side: "(t2 t3)"
//
//		Then, apply each hint on the relevant children of Nary join. In this
//		case, the relevant child of "t1" is CLogicalGet "t1" and the relevant
//		child of "(t2 t3)" is CLogicalLimit. Since CLogicalLimit is not a leaf
//		(i.e. has more than one relation), we recursively apply the right side
//		hint "(t2 t3)" on the CLogicalLimit expression.
//
//		After the children have been processed, then construct the appropriate
//		operator to join the children together.
//---------------------------------------------------------------------------
CExpression *
CJoinOrderHintsPreprocessor::RecursiveApplyJoinOrderHintsOnNAryJoin(
	CMemoryPool *mp, CExpression *pexpr, const CJoinHint::JoinNode *joinnode,
	std::set<ULONG> &usedChildrenIndexes, std::set<ULONG> &usedPredicateIndexes,
	ULONG *index)
{
	GPOS_ASSERT(COperator::EopLogicalNAryJoin == pexpr->Pop()->Eopid());

	CExpression *pexprAppliedHints = nullptr;

	StringPtrArray *hint_aliases = CHintUtils::GetAliasesFromHint(mp, joinnode);
	if (joinnode->GetName())
	{
		// Base case 1:
		//
		// hint specifies name, then return the child of CLogicalNAryJoin by
		// that name.
		pexprAppliedHints =
			GetChildWithSingleAlias(pexpr, joinnode->GetName(), index);
		if (nullptr != pexprAppliedHints)
		{
			pexprAppliedHints->AddRef();
		}
		hint_aliases->Release();
		usedChildrenIndexes.insert(*index);
		return pexprAppliedHints;
	}

	if (IsChild(mp, pexpr, hint_aliases))
	{
		// Base case 2:
		//
		// If a child covers the aliases in the jointree, then recursively
		// apply the hint to that child. For example, if child is a GROUP BY or
		// LIMIT expression, like:
		//
		// Leading(T1 T2)
		// SELECT * FROM (SELECT a FROM T1, T2 LIMIT 42) q, T3;
		//
		// Note: We already have a joinnode hint we are trying to satisfy. So,
		// explicity pass that one along so we don't try to find another
		// matching hint
		pexprAppliedHints = CJoinOrderHintsPreprocessor::PexprPreprocess(
			mp, GetChild(mp, pexpr, hint_aliases), joinnode);

		hint_aliases->Release();
		pexprAppliedHints = CJoinOrderHintsPreprocessor::PexprPreprocess(
			mp, GetChild(mp, pexpr, hint_aliases, index), joinnode);
		usedChildrenIndexes.insert(*index);
		return pexprAppliedHints;
	}

	// Recursive case:
	//
	// If no single child covers the hint, then apply the joinnode
	// inner/outer to the CLogicalNAryJoin. For example:
	//
	// Hint: Leading((T1 T2) (T3 T4))
	// Operator: NAryJoin [T1 T2 T3 T4]
	//
	// Then left (T1 T2) and right (T3 T4) hints need to be applied to the
	// operator, and the result joined.
	ULONG outer_index = MAX_INDEX;
	ULONG inner_index = MAX_INDEX;
	CExpression *outer = RecursiveApplyJoinOrderHintsOnNAryJoin(
		mp, pexpr, joinnode->GetOuter(), usedChildrenIndexes,
		usedPredicateIndexes, &outer_index);
	CExpression *inner = RecursiveApplyJoinOrderHintsOnNAryJoin(
		mp, pexpr, joinnode->GetInner(), usedChildrenIndexes,
		usedPredicateIndexes, &inner_index);

	usedChildrenIndexes.insert(outer_index);
	usedChildrenIndexes.insert(inner_index);

	// Hint not satisfied. This can happen, for example, if joins hint
	// splits across a GROUP BY, like:
	//
	// Leading(T1 T3)
	// SELECT * FROM (SELECT a FROM T1, T2 LIMIT 42) q, T3;
	//
	// It can also happen for invalid hints on LOJ/ROJ queries.
	if (outer == nullptr || inner == nullptr ||
		((outer_index == MAX_INDEX || inner_index == MAX_INDEX) &&
		 COperator::EopScalarNAryJoinPredList ==
			 (*pexpr)[pexpr->Arity() - 1]->Pop()->Eopid()))
	{
		hint_aliases->Release();
		pexpr->AddRef();
		CRefCount::SafeRelease(outer);
		CRefCount::SafeRelease(inner);

		return pexpr;
	}

	CExpressionArray *all_on_preds = CPredicateUtils::PdrgpexprConjuncts(
		mp, (*pexpr->PdrgPexpr())[pexpr->PdrgPexpr()->Size() - 1]);

	if (COperator::EopScalarNAryJoinPredList ==
		(*pexpr)[pexpr->Arity() - 1]->Pop()->Eopid())
	{
		if (outer_index != inner_index)
		{
			if (nullptr != index)
			{
				*index = std::min(outer_index, inner_index);
			}
			CExpression *pred =
				(*(*all_on_preds)[0])[std::max(outer_index, inner_index)];
			usedPredicateIndexes.insert(std::max(outer_index, inner_index));
			if (IsValidLOJPred(mp, outer, inner, pred))
			{
				pred->AddRef();
				if (outer_index < inner_index)
				{
					pexprAppliedHints = GPOS_NEW(mp)
						CExpression(mp, GPOS_NEW(mp) CLogicalLeftOuterJoin(mp),
									outer, inner, pred);
				}
				else
				{
					pexprAppliedHints = GPOS_NEW(mp)
						CExpression(mp, GPOS_NEW(mp) CLogicalRightOuterJoin(mp),
									outer, inner, pred);
				}
			}
			else
			{
				CRefCount::SafeRelease(outer);
				CRefCount::SafeRelease(inner);

				// Hint created an invalid plan - return nullptr
			}
		}
		else
		{
			GPOS_ASSERT(outer_index == inner_index);
			if (nullptr != index)
			{
				*index = outer_index;
			}

			CExpression *pred = (*(*all_on_preds)[0])[0];
			pred->AddRef();
			pexprAppliedHints = GPOS_NEW(mp) CExpression(
				mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), outer, inner, pred);
		}
	}
	else
	{
		pexprAppliedHints = GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), outer, inner,
						GetOnPreds(mp, outer, inner, all_on_preds));
	}
	all_on_preds->Release();


	hint_aliases->Release();
	return pexprAppliedHints;
}


//---------------------------------------------------------------------------
//	@function:
//		CJoinOrderHintsPreprocessor::PexprPreprocess
//
//	@doc:
//		Search for and apply join order hints on an expression.
//
//		After we have a hint there are 3 stages to apply the hint and construct
//		a new Nary join expression:
//
//		1. Apply the hint to the leaf operators.
//		2. Apply the hint to the non-leaf operators.
//		3. Add the unused children.
//
//		Steps 1 and 2 are handled in RecursiveApplyJoinOrderHintsOnNAryJoin().
//---------------------------------------------------------------------------
CExpression *
CJoinOrderHintsPreprocessor::PexprPreprocess(
	CMemoryPool *mp, CExpression *pexpr, const CJoinHint::JoinNode *joinnode)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	COperator *pop = pexpr->Pop();

	// Search for a join order hint for this expression.
	if (nullptr == joinnode)
	{
		CPlanHint *planhint =
			COptCtxt::PoctxtFromTLS()->GetOptimizerConfig()->GetPlanHint();

		CJoinHint *joinhint = planhint->GetJoinHint(pexpr);
		if (joinhint)
		{
			joinnode = joinhint->GetJoinNode();
		}
	}

	// Given a hint, recursively traverse the hint and (bottom-up) construct a
	// new join expression. Any leftover children are appended to the nary
	// join.
	if (COperator::EopLogicalNAryJoin == pop->Eopid() && nullptr != joinnode)
	{
		std::set<ULONG> usedPredicateIndexes;
		std::set<ULONG> usedChildrenIndexes;
		CExpression *pexprAppliedHints = RecursiveApplyJoinOrderHintsOnNAryJoin(
			mp, pexpr, joinnode, usedChildrenIndexes, usedPredicateIndexes);

		if (nullptr == pexprAppliedHints)
		{
			pexpr->AddRef();
			return pexpr;
		}

		CExpressionArray *naryChildren = ConstructNAryJoinChildren(
			mp, pexpr, pexprAppliedHints, usedChildrenIndexes,
			usedPredicateIndexes);
		if (naryChildren->Size() <= 1)
		{
			pexprAppliedHints->AddRef();
			naryChildren->Release();
			return pexprAppliedHints;
		}
		else if (COperator::EopScalarNAryJoinPredList ==
				 (*pexpr)[pexpr->Arity() - 1]->Pop()->Eopid())
		{
			CExpression *predList = (*pexpr)[pexpr->Arity() - 1];
			CExpressionArray *naryJoinPredicates =
				GPOS_NEW(mp) CExpressionArray(mp);
			ULongPtrArray *lojChildPredIndexes = GPOS_NEW(mp) ULongPtrArray(mp);
			for (ULONG ul = 0; ul < predList->Arity(); ul++)
			{
				if (usedPredicateIndexes.count(ul) == 0)
				{
					(*predList)[ul]->AddRef();
					naryJoinPredicates->Append((*predList)[ul]);
				}
			}
			for (ULONG ul = 0; ul < CLogicalNAryJoin::PopConvert(pexpr->Pop())
										->GetLojChildPredIndexes()
										->Size();
				 ul++)
			{
				ULONG predicateIndex =
					*(*CLogicalNAryJoin::PopConvert(pexpr->Pop())
						   ->GetLojChildPredIndexes())[ul];
				if (usedPredicateIndexes.count(predicateIndex) == 0)
				{
					// FIXME: this needs to be adjusted if middle index used
					//        example [0 1 3 2] and 1 is used then [0 2 1]
					ULONG adjustedPredicateIndex = predicateIndex;
					for (ULONG ul2 = 0; ul2 < predicateIndex; ul2++)
					{
						if (usedPredicateIndexes.count(ul2) != 0)
						{
							adjustedPredicateIndex -= 1;
						}
					}
					lojChildPredIndexes->Append(
						GPOS_NEW(mp) ULONG(adjustedPredicateIndex));
				}
			}
			naryChildren->Append(GPOS_NEW(mp) CExpression(
				mp, GPOS_NEW(mp) CScalarNAryJoinPredList(mp),
				naryJoinPredicates));

			return GPOS_NEW(mp) CExpression(
				mp, GPOS_NEW(mp) CLogicalNAryJoin(mp, lojChildPredIndexes),
				naryChildren);
		}
		else
		{
			(*pexpr->PdrgPexpr())[pexpr->PdrgPexpr()->Size() - 1]->AddRef();
			naryChildren->Append(
				(*pexpr->PdrgPexpr())[pexpr->PdrgPexpr()->Size() - 1]);

			return GPOS_NEW(mp) CExpression(
				mp, GPOS_NEW(mp) CLogicalNAryJoin(mp), naryChildren);
		}
	}

	// If either there is no hint or this not an nary join expression, then
	// recurse into our children.
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionArray *pdrgexprChildren = pexpr->PdrgPexpr();
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		pdrgpexpr->Append(CJoinOrderHintsPreprocessor::PexprPreprocess(
			mp, (*pdrgexprChildren)[ul], joinnode));
	}

	pop->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}
