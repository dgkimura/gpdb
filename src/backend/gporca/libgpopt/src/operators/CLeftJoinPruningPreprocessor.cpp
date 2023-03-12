//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2023 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLeftJoinPruningPreprocessor.cpp
//
//	@doc:
//		Preprocessing routines of left join pruning
//---------------------------------------------------------------------------

#include "gpopt/operators/CLeftJoinPruningPreprocessor.h"

#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"
#include "gpopt/search/CGroupExpression.h"


using namespace gpopt;

// This is the method to check if the left outer join can be pruned.If prunable
// it will return the new pruned expression else will return the old
// expression.  The checks performed in this method are -
// CheckJoinPruningCondOnInnerRel method checks that the output columns and
// the WHERE clause should only contain columns from the outer relation.
// CheckJoinPruningCondOnJoinCond method finds the unique keys of the inner
// relation that are either constant or equal to a column from the outer
// relation.  CheckForFullUniqueKeySetInInnerRel method checks if all the
// unique/primary keys from a particular key set collection of the inner
// relation are present in the join condition.
gpos::Ref<CExpression>
CLeftJoinPruningPreprocessor::PexprCheckLeftOuterJoinPruningConditions(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprNew,
	gpos::Ref<CColRefSet> output_columns)
{
	GPOS_ASSERT(nullptr != pexprNew);
	GPOS_ASSERT(nullptr != output_columns);

	//Check conditions on inner relation
	if (!CheckJoinPruningCondOnInnerRel(pexprNew, output_columns))
	{
		// return the current expression without pruning
		return pexprNew;
	}

	// Set to contain the unique keys of inner relation used in the join
	// condition which are either a constant or equal to some other column
	// from the outer relation
	gpos::Ref<CColRefSet> result = GPOS_NEW(mp) CColRefSet(mp);

	// Checking 'Join Conditions' to figure out if join is prunable.  If
	// the join condition contains an operator other than ScalarCmp(=)
	// , BooleanAnd or contains a subquery then it will return a false.  If
	// the above checks are passed then it will populate the 'result' with
	// unique keys of inner relation present in the join condition which
	// are either constant or equal to a column from the outer relation
	if (!CheckJoinPruningCondOnJoinCond(mp, pexprNew, result))
	{
		return pexprNew;
	}

	// If no unique keys are found in join condition implies join pruning
	// is not possible, so return the current expression
	if (result->Size() == 0)
	{
		return pexprNew;
	}

	// The 'result' contains all the unique keys which (A) are from the
	// inner relation and are part of the join condition (B) are either
	// constant or equal to some column from outer relation.  We check if
	// all the unique key of inner relation are present in 'result' if yes,
	// then we proceed for 'Join Pruning', and return Outer child only
	if (CheckForFullUniqueKeySetInInnerRel(mp, pexprNew, result))
	{
		gpos::Ref<CExpression> outer_child = (*pexprNew)[0];



		return outer_child;
	}
	else
	{
		return pexprNew;
	}
}

BOOL
CLeftJoinPruningPreprocessor::CheckJoinPruningCondOnInnerRel(
	const gpos::Ref<CExpression> pexprNew, gpos::Ref<CColRefSet> output_columns)
{
	GPOS_ASSERT(nullptr != pexprNew);
	GPOS_ASSERT(nullptr != output_columns);

	gpos::Ref<CExpression> inner_rel = (*pexprNew)[1];
	const gpos::Ref<CColRefSet> derive_output_columns_inner_rel =
		inner_rel->DeriveOutputColumns();

	// If the output_columns of CLogicalLeftOuterJoin operator contains
	// columns from the inner relation or inner relation has no unique keys
	// then join pruning is not done
	if (output_columns->FIntersects(derive_output_columns_inner_rel) ||
		nullptr == inner_rel->DeriveKeyCollection())
	{
		return false;
	}
	return true;
}

BOOL
CLeftJoinPruningPreprocessor::CheckJoinPruningCondOnJoinCond(
	CMemoryPool *mp, const gpos::Ref<CExpression> pexprNew,
	gpos::Ref<CColRefSet> result)
{
	GPOS_ASSERT(nullptr != pexprNew);
	GPOS_ASSERT(nullptr != result);

	// Outer relation of the left join
	gpos::Ref<CExpression> outer_rel = (*pexprNew)[0];

	// Inner relation of the left join
	gpos::Ref<CExpression> inner_rel = (*pexprNew)[1];

	// Join conditions of left join
	gpos::Ref<CExpression> join_cond = (*pexprNew)[2];

	gpos::Ref<CKeyCollection> inner_rel_key_sets =
		inner_rel->DeriveKeyCollection();

	// Unique keys of inner relation
	gpos::Ref<CColRefSet> inner_unique_keys = GPOS_NEW(mp) CColRefSet(mp);

	// Derived output columns of the outer relation of the left join
	gpos::Ref<CColRefSet> outer_rel_columns = outer_rel->DeriveOutputColumns();

	ULONG num_keys = inner_rel_key_sets->Keys();

	// Extracting unique keys of inner relation
	for (ULONG ul = 0; ul < num_keys; ul++)
	{
		gpos::Ref<CColRefSet> pdrgpcrKey = inner_rel_key_sets->PcrsKey(mp, ul);
		inner_unique_keys->Include(pdrgpcrKey);
	}

	// Join Pruning is done only when the join condition is a 'ScalarCmp
	// (=)' or 'BOOLEAN AND'. No pruning is done if a subquery is present
	// in the join condition.
	if ((!CPredicateUtils::IsEqualityOp(join_cond) &&
		 !CPredicateUtils::FAnd(join_cond)) ||
		join_cond->DeriveHasSubquery())
	{
		return false;
	}

	// Condition 1:  'ScalarCmp (=)' operator present in the join condition
	if (CPredicateUtils::IsEqualityOp(join_cond))
	{
		// If the Equality expression contains expressions other than ident
		// and const then join pruning will not be done
		if (!CPredicateUtils::FPlainEqualityIdentConstWithoutCast(join_cond))
		{
			return false;
		}

		// Join pruning will not happen if the join condition is of the
		// form table1.col1=table1.col1
		if (CPredicateUtils::FPlainEquality(join_cond) &&
			1 == join_cond->DeriveUsedColumns()->Size())
		{
			return false;
		}

		// Contains columns from a join condition that are present in the
		// inner relation
		gpos::Ref<CColRefSet> inner_columns = GPOS_NEW(mp) CColRefSet(mp);

		// Columns used in a join condition
		gpos::Ref<CColRefSet> usedColumns = join_cond->DeriveUsedColumns();

		CheckUniqueKeyInJoinCond(inner_columns, usedColumns, result,
								 inner_unique_keys, outer_rel_columns);
	}

	// Condition 2: If AND operator is present in join condition
	if (CPredicateUtils::FAnd(join_cond) &&
		!CheckAndCondInJoinCond(mp, join_cond, inner_unique_keys, result,
								outer_rel_columns))
	{
		return false;
	}


	return true;
}

BOOL
CLeftJoinPruningPreprocessor::CheckAndCondInJoinCond(
	CMemoryPool *mp, const gpos::Ref<CExpression> join_cond,
	const gpos::Ref<CColRefSet> inner_unique_keys, gpos::Ref<CColRefSet> result,
	const gpos::Ref<CColRefSet> outer_rel_columns)
{
	GPOS_ASSERT(nullptr != join_cond);
	GPOS_ASSERT(nullptr != inner_unique_keys);
	GPOS_ASSERT(nullptr != result);
	GPOS_ASSERT(nullptr != outer_rel_columns);

	ULONG arity_join = join_cond->Arity();
	for (ULONG ul = 0; ul < arity_join; ul++)
	{
		gpos::Ref<CExpression> join_cond_child = (*join_cond)[ul];

		// If the child of the ScalarBoolAnd is not a ScalarCmp then
		// join pruning is not possible
		if (!CUtils::FScalarCmp(join_cond_child))
		{
			return false;
		}

		// If the child is a ScalarCmp but not an Equality then continue and also
		// if the Equality expression contains expressions other than ident
		// and const then continue
		if (!CPredicateUtils::FPlainEqualityIdentConstWithoutCast(
				join_cond_child))
		{
			continue;
		}

		// If the join condition is like table1.col1=table1.col1 then continue
		if (CPredicateUtils::FPlainEquality(join_cond_child) &&
			1 == join_cond_child->DeriveUsedColumns()->Size())
		{
			continue;
		}

		// Contains columns from a join condition that are present in
		// the inner relation
		gpos::Ref<CColRefSet> inner_columns = GPOS_NEW(mp) CColRefSet(mp);

		// Columns used in a join condition
		gpos::Ref<CColRefSet> usedColumns =
			join_cond_child->DeriveUsedColumns();

		CheckUniqueKeyInJoinCond(inner_columns, usedColumns, result,
								 inner_unique_keys, outer_rel_columns);
	}
	return true;
}

BOOL
CLeftJoinPruningPreprocessor::CheckForFullUniqueKeySetInInnerRel(
	CMemoryPool *mp, const gpos::Ref<CExpression> pexprNew,
	const gpos::Ref<CColRefSet> result)
{
	GPOS_ASSERT(nullptr != pexprNew);
	GPOS_ASSERT(nullptr != result);

	gpos::Ref<CExpression> inner_rel = (*pexprNew)[1];
	gpos::Ref<CKeyCollection> inner_rel_key_sets =
		inner_rel->DeriveKeyCollection();
	ULONG ulKeys = inner_rel_key_sets->Keys();
	BOOL are_all_unique_keys_present;
	for (ULONG ul = 0; ul < ulKeys; ul++)
	{
		gpos::Ref<CColRefSet> pdrgpcrKey = inner_rel_key_sets->PcrsKey(mp, ul);
		CColRefSetIter it(*pdrgpcrKey);
		are_all_unique_keys_present = true;
		while (it.Advance())
		{
			gpos::Ref<CColRef> pcr = it.Pcr();
			if (!result->FMember(pcr))
			{
				are_all_unique_keys_present = false;
				break;
			}
		}

		if (are_all_unique_keys_present)
		{
			break;
		}
	}
	return are_all_unique_keys_present;
}


// This method checks if any of the columns used in an Equality ScalarCmp
// operator belongs to the inner relation of the left join.If the column
// from the inner relation is equal to a constant or a column from the
// outer relation and is also a unique key of the inner relation then it
// is inserted in the result set.
void
CLeftJoinPruningPreprocessor::CheckUniqueKeyInJoinCond(
	gpos::Ref<CColRefSet> inner_columns,
	const gpos::Ref<CColRefSet> usedColumns, gpos::Ref<CColRefSet> result,
	const gpos::Ref<CColRefSet> inner_unique_keys,
	const gpos::Ref<CColRefSet> outer_rel_columns)
{
	GPOS_ASSERT(nullptr != inner_columns);
	GPOS_ASSERT(nullptr != usedColumns);
	GPOS_ASSERT(nullptr != result);
	GPOS_ASSERT(nullptr != inner_unique_keys);
	GPOS_ASSERT(nullptr != outer_rel_columns);

	// If the used column count is 1 then we need to check if that column is
	// a part of the inner relation unique key
	if (usedColumns->Size() == 1 &&
		inner_unique_keys->FMember(usedColumns->PcrFirst()))
	{
		result->Include(usedColumns->PcrFirst());
	}

	// If the used column count is 2, we need to check from those two
	// columns which one belong to inner relation
	if (usedColumns->Size() == 2)
	{
		CColRefSetIter it(*usedColumns);

		while (it.Advance())
		{
			gpos::Ref<CColRef> pcr = it.Pcr();
			if (!outer_rel_columns->FMember(pcr))
			{
				inner_columns->Include(pcr);
			}
		}

		// This case means we have one column from the outer relation
		// and one column from the inner relation.We need to check if
		// the column from the inner relation is a part of the unique
		// key or not.
		if (inner_columns->Size() == 1 &&
			inner_unique_keys->FMember(inner_columns->PcrFirst()))
		{
			result->Include(inner_columns->PcrFirst());
		}
	}
}


// This method is called by JoinPruningTreeTraversal method whenever a scalar
// operator is encountered during the tree traversal.  This method checks if
// any scalar subquery is present in the tree and find the output column of that
// subquery (subquery_colref) and calls back JoinPruningTreeTraversal method
// to prune any left join present in the subquery.
gpos::Ref<CExpression>
CLeftJoinPruningPreprocessor::PexprJoinPruningScalarSubquery(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar)
{
	GPOS_ASSERT(nullptr != pexprScalar);
	GPOS_ASSERT(pexprScalar->Pop()->FScalar());

	if (CUtils::FExistentialSubquery(pexprScalar->Pop()))
	{
		return pexprScalar;
	}

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CColRefSet> subquery_colrefset = GPOS_NEW(mp) CColRefSet(mp);
	gpos::Ref<CColRef> subquery_colref = nullptr;
	if (COperator::EopScalarSubquery == pexprScalar->Pop()->Eopid())
	{
		gpos::Ref<CScalarSubquery> subquery_pop =
			CScalarSubquery::PopConvert(pexprScalar->Pop());
		subquery_colref = subquery_pop->Pcr();
	}
	else if (CUtils::FQuantifiedSubquery(pexprScalar->Pop()))
	{
		gpos::Ref<CScalarSubqueryQuantified> subquery_pop =
			CScalarSubqueryQuantified::PopConvert(pexprScalar->Pop());
		subquery_colref = subquery_pop->Pcr();
	}

	// If we have a subquery inside CScalarCmp then subquery_colref will be null
	if (nullptr != subquery_colref)
	{
		subquery_colrefset->Include(subquery_colref);
	}

	pexprScalar = JoinPruningTreeTraversal(mp, pexprScalar, pdrgpexpr,
										   subquery_colrefset);

	return pexprScalar;
}

// This method is used to find the output columns of the expression passed and
// also the output columns of the child tree of the expression. These values
// are calculated so that while processing the CLogicalLeftOuterJoin operator
// in 'CheckJoinPruningCondOnInnerRel' method we can check that all the output
// columns of the CLogicalLeftOuterJoin operator should be from the outer
// relation.
void
CLeftJoinPruningPreprocessor::ComputeOutputColumns(
	const gpos::Ref<CExpression> pexpr,
	const gpos::Ref<CColRefSet> derived_output_columns,
	gpos::Ref<CColRefSet> output_columns,
	gpos::Ref<CColRefSet> childs_output_columns,
	const gpos::Ref<CColRefSet> pcrsOutput)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != derived_output_columns);
	GPOS_ASSERT(nullptr != output_columns);
	GPOS_ASSERT(nullptr != childs_output_columns);

	// Computing output columns of the parent
	CColRefSetIter iter_derived_output_columns(*derived_output_columns);
	while (iter_derived_output_columns.Advance())
	{
		gpos::Ref<CColRef> pcr = iter_derived_output_columns.Pcr();
		if (pcrsOutput->FMember(pcr))
		{
			output_columns->Include(pcr);
		}
	}

	// Computing output columns of the child tree
	ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexpr_child = (*pexpr)[ul];
		if (pexpr_child->Pop()->FScalar())
		{
			gpos::Ref<CColRefSet> derived_used_columns_scalar =
				pexpr_child->DeriveUsedColumns();
			childs_output_columns->Include(output_columns);
			childs_output_columns->Include(derived_used_columns_scalar);
		}
	}
}

gpos::Ref<CExpression>
CLeftJoinPruningPreprocessor::JoinPruningTreeTraversal(
	CMemoryPool *mp, const gpos::Ref<CExpression> pexpr,
	gpos::Ref<CExpressionArray> pdrgpexpr,
	const gpos::Ref<CColRefSet> childs_output_columns)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pdrgpexpr);

	ULONG number_of_childs = pexpr->Arity();
	for (ULONG ul = 0; ul < number_of_childs; ul++)
	{
		gpos::Ref<CExpression> pexpr_child = (*pexpr)[ul];
		if (pexpr_child->Pop()->FLogical())
		{
			gpos::Ref<CExpression> pexprLogicalJoinPrunedChild =
				PexprPreprocess(mp, pexpr_child, childs_output_columns);
			pdrgpexpr->Append(pexprLogicalJoinPrunedChild);
		}
		else if (pexpr_child->DeriveHasSubquery())
		{
			gpos::Ref<CExpression> pexprScalarJoinPrunedChild =
				PexprJoinPruningScalarSubquery(mp, pexpr_child);
			pdrgpexpr->Append(pexprScalarJoinPrunedChild);
		}
		else
		{
			pdrgpexpr->Append(pexpr_child);
		}
	}

	gpos::Ref<COperator> pop = pexpr->Pop();

	gpos::Ref<CExpression> pexprNew =
		GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
	return pexprNew;
}


// This method is used to prune the CLogicalLeftOuterJoin operator in a bottom
// up approach based on the following conditions
// 1. The output columns and the WHERE clause should only contain columns from
// the outer relation.
// 2. The unique keys of the inner relation should either be constant or equal
// to a column from the outer relation and the join condition should contain only
// ScalarCmp or BooleanAnd op.
// 3. All the unique/primary keys from a particular key set collection of the inner
// relation should be present in the join condition.
// example:
//     create table foo (a int,b int,c int, d int,e int,constraint idx1 unique(a,b)
//     ,constraint idx2 unique(a,c,d));
//     create table bar (p int,q int,r int, s int,t int,constraint idx3 unique(p,q)
//     ,constraint idx4 unique(p,r,s));
//     Keyset Collection of foo: [a,b],[a,c,d]
//     KeySet Collection of bar: [p,q],[p,r,s]
//
// Query: explain select foo.* from foo left join bar on foo.a=bar.p and foo.c=bar.r
// and bar.s=200 where foo.e>100;
//
// Since the o/p columns and the WHERE clause use columns from the outer relation
// foo and the keyset collection [p,r,s] of the inner table is present in the join
// condition which is a constant or equal to a column of outer relation,the join can
// be pruned. When the join is pruned the outer child of the CLogicalLeftOuterJoin is
// returned to the parent operator.

// Algebrized query:
// +--CLogicalSelect
//    |--CLogicalLeftOuterJoin
//    |  |--CLogicalGet "foo" ("foo"), Columns: ["a" (0), "b" (1), "c" (2), "d" (3), "e" (4)] Key sets: {[0,1], [0,2,3], [5,11]}
//    |  |--CLogicalGet "bar" ("bar"), Columns: ["p" (12), "q" (13), "r" (14), "s" (15), "t" (16)] Key sets: {[0,1], [0,2,3], [5,11]}
//    |  +--CScalarBoolOp (EboolopAnd)
//    |     |--CScalarCmp (=)
//    |     |  |--CScalarIdent "a" (0)
//    |     |  +--CScalarIdent "p" (12)
//    |     |--CScalarCmp (=)
//    |     |  |--CScalarIdent "c" (2)
//    |     |  +--CScalarIdent "r" (14)
//    |     +--CScalarCmp (=)
//    |        |--CScalarIdent "s" (15)
//    |        +--CScalarConst (200)
//    +--CScalarCmp (>)
//       |--CScalarIdent "e" (4)
//       +--CScalarConst (100)


// Algebrized preprocessed query:
// +--CLogicalSelect
//    |--CLogicalGet "foo" ("foo"), Columns: ["a" (0), "b" (1), "c" (2), "d" (3), "e" (4)] Key sets: {[0,1], [0,2,3], [5,11]}
//    +--CScalarCmp (>)
//       |--CScalarIdent "e" (4)
//       +--CScalarConst (100)

gpos::Ref<CExpression>
CLeftJoinPruningPreprocessor::PexprPreprocess(
	CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
	const gpos::Ref<CColRefSet> pcrsOutput)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(pexpr->Pop()->FLogical());

	if (nullptr == pcrsOutput)
	{
		return pexpr;
	}

	// Derived output columns of the current expression
	gpos::Ref<CColRefSet> derived_output_columns = pexpr->DeriveOutputColumns();

	// Columns which are output by the current expression
	gpos::Ref<CColRefSet> output_columns = GPOS_NEW(mp) CColRefSet(mp);

	// Columns which are output by the child tree of the current expression
	gpos::Ref<CColRefSet> childs_output_columns = GPOS_NEW(mp) CColRefSet(mp);

	// Computing output columns of the current expression (output_columns)
	// and the output columns of child tree (childs_output_columns)
	ComputeOutputColumns(pexpr, derived_output_columns, output_columns,
						 childs_output_columns, pcrsOutput);

	// Array to hold the child expressions
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	// Traversing the tree
	gpos::Ref<CExpression> pexprNew =
		JoinPruningTreeTraversal(mp, pexpr, pdrgpexpr, childs_output_columns);


	// Checking if the join is a left join then pruning the join if possible
	if (CPredicateUtils::FLeftOuterJoin(pexprNew))
	{
		// If the pruning condition are met, then pexprNew shall contain
		// pruned expression else the old expr is retained
		pexprNew = PexprCheckLeftOuterJoinPruningConditions(mp, pexprNew,
															output_columns);
	}

	return pexprNew;
}

//EOF
