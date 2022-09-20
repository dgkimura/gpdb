//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2011 EMC Corp.
//
//	@filename:
//		CPredicateUtils.h
//
//	@doc:
//		Utility functions for normalizing scalar predicates
//---------------------------------------------------------------------------
#ifndef GPOPT_CPredicateUtils_H
#define GPOPT_CPredicateUtils_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CScalarBoolOp.h"

namespace gpopt
{
using namespace gpos;

// fwd decl
class CConstraintInterval;

//---------------------------------------------------------------------------
//	@class:
//		CPredicateUtils
//
//	@doc:
//		Utility functions for normalizing scalar predicates
//
//---------------------------------------------------------------------------
class CPredicateUtils
{
private:
	// collect conjuncts recursively
	static void CollectConjuncts(gpos::pointer<CExpression *> pexpr,
								 gpos::pointer<CExpressionArray *> pdrgpexpr);

	// collect disjuncts recursively
	static void CollectDisjuncts(gpos::pointer<CExpression *> pexpr,
								 gpos::pointer<CExpressionArray *> pdrgpexpr);

	// check if a conjunct/disjunct can be skipped
	static BOOL FSkippable(gpos::pointer<CExpression *> pexpr,
						   BOOL fConjunction);

	// check if a conjunction/disjunction can be reduced to a constant True/False
	static BOOL FReducible(gpos::pointer<CExpression *> pexpr,
						   BOOL fConjunction);

	// check if given expression is a comparison over the given column
	static BOOL FComparison(gpos::pointer<CExpression *> pexpr, CColRef *colref,
							gpos::pointer<CColRefSet *> pcrsAllowedRefs);

	// check whether the given expression contains references to only the given
	// columns. If pcrsAllowedRefs is NULL, then check whether the expression has
	// no column references and no volatile functions
	static BOOL FValidRefsOnly(gpos::pointer<CExpression *> pexprScalar,
							   gpos::pointer<CColRefSet *> pcrsAllowedRefs);

	// determine which predicates we should test implication for
	static BOOL FCheckPredicateImplication(
		gpos::pointer<CExpression *> pexprPred);

	// check if predicate is implied by given equivalence classes
	static BOOL FImpliedPredicate(
		gpos::pointer<CExpression *> pexprPred,
		gpos::pointer<CColRefSetArray *> pdrgpcrsEquivClasses);

	// helper to create index lookup comparison predicate with index key on left side
	static gpos::owner<CExpression *> PexprIndexLookupKeyOnLeft(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CExpression *> pexprScalar,
		gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CColRefArray *> pdrgpcrIndex,
		gpos::pointer<CColRefSet *> outer_refs);

	// helper to create index lookup comparison predicate with index key on right side
	static gpos::owner<CExpression *> PexprIndexLookupKeyOnRight(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CExpression *> pexprScalar,
		gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CColRefArray *> pdrgpcrIndex,
		gpos::pointer<CColRefSet *> outer_refs);

	// for all columns that appear in the given expression and are members
	// of the given set, replace these columns with NULL constants
	static gpos::owner<CExpression *> PexprReplaceColsWithNulls(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprScalar,
		CColRefSet *pcrs);

public:
	// private dtor
	virtual ~CPredicateUtils() = delete;

	// private ctor
	CPredicateUtils() = delete;

	CPredicateUtils(const CPredicateUtils &) = delete;

	// reverse the comparison, for example "<" => ">", "<=" => "=>
	static IMDType::ECmpType EcmptReverse(IMDType::ECmpType cmp_type);

	// check if the expression is a boolean scalar identifier
	static BOOL FBooleanScalarIdent(gpos::pointer<CExpression *> pexprPred);

	// check if the expression is a negated boolean scalar identifier
	static BOOL FNegatedBooleanScalarIdent(
		gpos::pointer<CExpression *> pexprPred);

	// is the given expression an equality comparison
	static BOOL IsEqualityOp(gpos::pointer<CExpression *> pexpr);

	// is the given expression a scalar comparison
	static BOOL FComparison(gpos::pointer<CExpression *> pexpr);

	// is the given expression a conjunction of equality comparisons
	static BOOL FConjunctionOfEqComparisons(CMemoryPool *mp,
											gpos::pointer<CExpression *> pexpr);

	// is the given expression a comparison of the given type
	static BOOL FComparison(gpos::pointer<CExpression *> pexpr,
							IMDType::ECmpType cmp_type);

	// is the given array of expressions contain a volatile function like random()
	static BOOL FContainsVolatileFunction(
		gpos::pointer<CExpressionArray *> pdrgpexpr);

	// is the given expressions contain a volatile function like random()
	static BOOL FContainsVolatileFunction(gpos::pointer<CExpression *> pexpr);

	// is the given expression an equality comparison over two scalar identifiers
	static BOOL FPlainEquality(gpos::pointer<CExpression *> pexpr);

	// is the given expression a self comparison on some column
	static BOOL FSelfComparison(gpos::pointer<CExpression *> pexpr,
								IMDType::ECmpType *pecmpt);

	// eliminate self comparison if possible
	static gpos::owner<CExpression *> PexprEliminateSelfComparison(
		CMemoryPool *mp, CExpression *pexpr);

	// is the given expression in the form (col1 Is NOT DISTINCT FROM col2)
	static BOOL FINDFScalarIdents(gpos::pointer<CExpression *> pexpr);

	// is the given expression in the form (col1 IS DISTINCT FROM col2)
	static BOOL FIDFScalarIdents(gpos::pointer<CExpression *> pexpr);

	// check if expression is an Is DISTINCT FROM FALSE expression
	static BOOL FIDFFalse(gpos::pointer<CExpression *> pexpr);

	static BOOL FCompareScalarIdentToConstAndScalarIdentArray(
		gpos::pointer<CExpression *> pexpr);

	// is the given expression in the form (expr IS DISTINCT FROM expr)
	static BOOL FIDF(gpos::pointer<CExpression *> pexpr);

	// is the given expression in the form (expr IS NOT DISTINCT FROM expr)
	static BOOL FINDF(gpos::pointer<CExpression *> pexpr);

	// generate a conjunction of INDF expressions between corresponding columns in the given arrays
	static gpos::owner<CExpression *> PexprINDFConjunction(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> pdrgpcrFirst,
		gpos::pointer<CColRefArray *> pdrgpcrSecond);

	// is the given expression of the form (col CMP/IS DISTINCT/IS NOT DISTINCT FROM constant)
	// either the constant or the column can be casted
	static BOOL FIdentCompareConstIgnoreCast(gpos::pointer<CExpression *> pexpr,
											 COperator::EOperatorId);

	// check if a given expression is a comparison between a column and an expression
	// using only constants and one or more outer references from a given ColRefSet,
	// it optionally returns the colref of the local table
	static BOOL FIdentCompareOuterRefExprIgnoreCast(
		gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefSet *> pcrsOuterRefs,
		CColRef **localColRef = nullptr);

	// is the given expression a comparison between scalar ident and a const array
	// either the ident or constant array can be casted
	static BOOL FArrayCompareIdentToConstIgnoreCast(
		gpos::pointer<CExpression *> pexpr);

	// is the given expression of the form (col IS DISTINCT FROM constant)
	// either the constant or the column can be casted
	static BOOL FIdentIDFConstIgnoreCast(gpos::pointer<CExpression *> pexpr);

	// is the given expression of the form NOT (col IS DISTINCT FROM constant)
	// either the constant or the column can be casted
	static BOOL FIdentINDFConstIgnoreCast(gpos::pointer<CExpression *> pexpr);

	// is the given expression a comparison between a scalar ident and a constant
	static BOOL FCompareIdentToConst(gpos::pointer<CExpression *> pexpr);

	// is the given expression a comparison between a const and a const
	static BOOL FCompareConstToConstIgnoreCast(
		gpos::pointer<CExpression *> pexpr);

	// is the given expression of the form (col IS DISTINCT FROM const)
	static BOOL FIdentIDFConst(gpos::pointer<CExpression *> pexpr);

	static BOOL FEqIdentsOfSameType(gpos::pointer<CExpression *> pexpr);

	// checks if comparison is between two columns, or a column and a const
	static BOOL FCompareColToConstOrCol(
		gpos::pointer<CExpression *> pexprScalar);

	// is the given expression a comparison between a scalar ident under a scalar cast and a constant array
	static BOOL FCompareCastIdentToConstArray(
		gpos::pointer<CExpression *> pexpr);

	// is the given expression a comparison between a scalar ident and a constant array
	static BOOL FCompareIdentToConstArray(gpos::pointer<CExpression *> pexpr);

	// is the given expression an AND
	static BOOL
	FAnd(gpos::pointer<CExpression *> pexpr)
	{
		return CUtils::FScalarBoolOp(pexpr, CScalarBoolOp::EboolopAnd);
	}

	// is the given expression an OR
	static BOOL
	FOr(gpos::pointer<CExpression *> pexpr)
	{
		return CUtils::FScalarBoolOp(pexpr, CScalarBoolOp::EboolopOr);
	}

	// does the given expression have any NOT children?
	// is the given expression a NOT
	static BOOL
	FNot(gpos::pointer<CExpression *> pexpr)
	{
		return CUtils::FScalarBoolOp(pexpr, CScalarBoolOp::EboolopNot);
	}

	// returns true iff the given expression is a Not operator whose child is a
	// scalar identifier
	static BOOL FNotIdent(gpos::pointer<CExpression *> pexpr);

	// does the given expression have any NOT children?
	static BOOL FHasNegatedChild(gpos::pointer<CExpression *> pexpr);

	// is the given expression an inner join or NAry join
	static BOOL
	FInnerOrNAryJoin(gpos::pointer<CExpression *> pexpr)
	{
		return COperator::EopLogicalInnerJoin == pexpr->Pop()->Eopid() ||
			   COperator::EopLogicalNAryJoin == pexpr->Pop()->Eopid();
	}

	static BOOL
	FLeftOuterJoin(gpos::pointer<CExpression *> pexpr)
	{
		return COperator::EopLogicalLeftOuterJoin == pexpr->Pop()->Eopid();
	}

	// is the given expression either a union or union all operator
	static BOOL
	FUnionOrUnionAll(gpos::pointer<CExpression *> pexpr)
	{
		return COperator::EopLogicalUnion == pexpr->Pop()->Eopid() ||
			   COperator::EopLogicalUnionAll == pexpr->Pop()->Eopid();
	}

	// check if we can collapse the nth child of the given union / union all operator
	static BOOL FCollapsibleChildUnionUnionAll(
		gpos::pointer<CExpression *> pexprParent, ULONG child_index);

	// if the nth child of the given union/union all expression is also an union / union all expression,
	// then collect the later's children and set the input columns of the new n-ary union/unionall operator
	static void CollectGrandChildrenUnionUnionAll(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, ULONG child_index,
		gpos::pointer<CExpressionArray *> pdrgpexprResult,
		gpos::pointer<CColRef2dArray *> pdrgdrgpcrResult);

	// extract conjuncts from a scalar tree
	static gpos::owner<CExpressionArray *> PdrgpexprConjuncts(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// extract disjuncts from a scalar tree
	static gpos::owner<CExpressionArray *> PdrgpexprDisjuncts(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// extract equality predicates on scalar identifier from a list of scalar expressions
	static gpos::owner<CExpressionArray *> PdrgpexprPlainEqualities(
		CMemoryPool *mp, gpos::pointer<CExpressionArray *> pdrgpexpr);

	// create conjunction/disjunction
	static gpos::owner<CExpression *> PexprConjDisj(
		CMemoryPool *mp, gpos::owner<CExpressionArray *> Pdrgpexpr,
		BOOL fConjunction);

	// create conjunction/disjunction of two expressions
	static gpos::owner<CExpression *> PexprConjDisj(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOne,
		gpos::pointer<CExpression *> pexprTwo, BOOL fConjunction);

	// create conjunction
	static gpos::owner<CExpression *> PexprConjunction(
		CMemoryPool *mp, gpos::owner<CExpressionArray *> pdrgpexpr);

	// create conjunction of two expressions
	static gpos::owner<CExpression *> PexprConjunction(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOne,
		gpos::pointer<CExpression *> pexprTwo);

	// create disjunction of two expressions
	static gpos::owner<CExpression *> PexprDisjunction(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOne,
		gpos::pointer<CExpression *> pexprTwo);

	// expand disjuncts in the given array by converting ArrayComparison to AND/OR tree and deduplicating resulting disjuncts
	static gpos::owner<CExpressionArray *> PdrgpexprExpandDisjuncts(
		CMemoryPool *mp, gpos::pointer<CExpressionArray *> pdrgpexprDisjuncts);

	// expand conjuncts in the given array by converting ArrayComparison to AND/OR tree and deduplicating resulting conjuncts
	static gpos::owner<CExpressionArray *> PdrgpexprExpandConjuncts(
		CMemoryPool *mp, gpos::pointer<CExpressionArray *> pdrgpexprConjuncts);

	// check if the given expression is a boolean expression on the
	// given column, e.g. if its of the form "ScalarIdent(colref)" or
	// "Not(ScalarIdent(colref))"
	static BOOL FBoolPredicateOnColumn(gpos::pointer<CExpression *> pexpr,
									   CColRef *colref);

	// check if the given expression is a scalar array cmp expression on the
	// given column
	static BOOL FScArrayCmpOnColumn(gpos::pointer<CExpression *> pexpr,
									CColRef *colref, BOOL fConstOnly);

	// check if the given expression is a null check on the given column
	// i.e. "is null" or "is not null"
	static BOOL FNullCheckOnColumn(gpos::pointer<CExpression *> pexpr,
								   CColRef *colref);

	// check if the given expression of the form "col is not null"
	static BOOL FNotNullCheckOnColumn(gpos::pointer<CExpression *> pexpr,
									  CColRef *colref);

	// check if the given expression is a disjunction of scalar cmp
	// expressions on the given column
	static BOOL IsDisjunctionOfRangeComparison(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		gpos::pointer<CColRefSet *> pcrsAllowedRefs, BOOL allowNotEqualPreds);

	// check if the given comparison type is one of the range comparisons, i.e.
	// LT, GT, LEq, GEq, Eq
	static BOOL FRangeComparison(gpos::pointer<CExpression *> expr,
								 CColRef *colref,
								 gpos::pointer<CColRefSet *> pcrsAllowedRefs,
								 BOOL allowNotEqualPreds);

	// create disjunction
	static gpos::owner<CExpression *> PexprDisjunction(
		CMemoryPool *mp, gpos::owner<CExpressionArray *> Pdrgpexpr);

	static gpos::owner<CExpression *> ValidatePartPruningExpr(
		CMemoryPool *mp, gpos::pointer<CExpression *> expr, CColRef *pcrPartKey,
		gpos::pointer<CColRefSet *> pcrsAllowedRefs,
		BOOL allow_not_equals_preds);

	// find a predicate that can be used for partition pruning with the given part key
	static gpos::owner<CExpression *> PexprPartPruningPredicate(
		CMemoryPool *mp, gpos::pointer<const CExpressionArray *> pdrgpexpr,
		CColRef *pcrPartKey, CExpression *pexprCol,
		gpos::pointer<CColRefSet *> pcrsAllowedRefs, BOOL allowNotEqualPreds);

	// append the conjuncts from the given expression to the given array, removing
	// any duplicates, and return the resulting array
	static gpos::owner<CExpressionArray *> PdrgpexprAppendConjunctsDedup(
		CMemoryPool *mp, gpos::pointer<CExpressionArray *> pdrgpexpr,
		gpos::pointer<CExpression *> pexpr);

	// extract interesting conditions involving the partitioning keys
	static gpos::owner<CExpression *> PexprExtractPredicatesOnPartKeys(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprScalar,
		gpos::pointer<CColRef2dArray *> pdrgpdrgpcrPartKeys,
		gpos::pointer<CColRefSet *> pcrsAllowedRefs, BOOL fUseConstraints,
		gpos::pointer<const IMDRelation *> pmdrel = nullptr);

	// extract the constraint on the given column and return the corresponding
	// scalar expression
	static CExpression *PexprPredicateCol(CMemoryPool *mp,
										  gpos::pointer<CConstraint *> pcnstr,
										  CColRef *colref,
										  BOOL fUseConstraints);

	// extract components of a comparison expression on the given key
	static void ExtractComponents(gpos::pointer<CExpression *> pexprScCmp,
								  CColRef *pcrKey,
								  gpos::pointer<CExpression *> *ppexprKey,
								  gpos::pointer<CExpression *> *ppexprOther,
								  IMDType::ECmpType *pecmpt);

	// Expression is a comparison with a simple identifer on at least one side
	static BOOL FIdentCompare(gpos::pointer<CExpression *> pexpr,
							  IMDType::ECmpType pecmpt, CColRef *colref);

	// check if given column is a constant
	static BOOL FConstColumn(gpos::pointer<CConstraint *> pcnstr,
							 const CColRef *colref);

	// check if given column is a disjunction of constants
	static BOOL FColumnDisjunctionOfConst(gpos::pointer<CConstraint *> pcnstr,
										  const CColRef *colref);

	// check if given column is a disjunction of constants
	static BOOL FColumnDisjunctionOfConst(
		gpos::pointer<CConstraintInterval *> pcnstr, const CColRef *colref);

	// split predicates into those that refer to an index key, and those that don't
	static void ExtractIndexPredicates(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CExpressionArray *> pdrgpexprPredicate,
		gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CColRefArray *> pdrgpcrIndex,
		CExpressionArray *pdrgpexprIndex, CExpressionArray *pdrgpexprResidual,
		gpos::pointer<CColRefSet *> pcrsAcceptedOuterRefs =
			nullptr,  // outer refs that are acceptable in an index predicate
		BOOL allowArrayCmpForBTreeIndexes = false);

	// return the inverse of given comparison expression
	static gpos::owner<CExpression *> PexprInverseComparison(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprCmp);

	// prune unnecessary equality operations
	static gpos::owner<CExpression *> PexprPruneSuperfluosEquality(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// remove conjuncts that are implied based on child columns equivalence classes
	static gpos::owner<CExpression *> PexprRemoveImpliedConjuncts(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprScalar,
		CExpressionHandle &exprhdl);

	//	check if given correlations are valid for semi join operator;
	static BOOL FValidSemiJoinCorrelations(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprInner,
		gpos::pointer<CExpressionArray *> pdrgpexprCorrelations);

	// check if given expression is composed of simple column equalities that use columns from given column set
	static BOOL FSimpleEqualityUsingCols(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprScalar,
		gpos::pointer<CColRefSet *> pcrs);

	// check if given expression is a valid index lookup predicate
	// return (modified) predicate suited for index lookup
	static gpos::owner<CExpression *> PexprIndexLookup(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CExpression *> pexpPred,
		gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CColRefArray *> pdrgpcrIndex,
		gpos::pointer<CColRefSet *> outer_refs,
		BOOL allowArrayCmpForBTreeIndexes);

	// split given scalar expression into two conjunctions; without and with outer references
	static void SeparateOuterRefs(CMemoryPool *mp, CExpression *pexprScalar,
								  CColRefSet *outer_refs,
								  gpos::owner<CExpression *> *ppexprLocal,
								  gpos::owner<CExpression *> *ppexprOuterRef);

	// is the condition a LIKE predicate
	static BOOL FLikePredicate(gpos::pointer<IMDId *> mdid);

	// is the condition a LIKE predicate
	static BOOL FLikePredicate(gpos::pointer<CExpression *> pexprPred);

	// extract the components of a LIKE predicate
	static void ExtractLikePredComponents(
		gpos::pointer<CExpression *> pexprPred, CExpression **ppexprScIdent,
		CExpression **ppexprConst);

	// check if scalar expression is null-rejecting and uses columns from given column set
	static BOOL FNullRejecting(CMemoryPool *mp,
							   gpos::pointer<CExpression *> pexprScalar,
							   gpos::pointer<CColRefSet *> pcrs);

	// returns true iff the given predicate pexprPred is compatible with the given index pmdindex
	static BOOL FCompatibleIndexPredicate(
		gpos::pointer<CExpression *> pexprPred,
		gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CColRefArray *> pdrgpcrIndex, CMDAccessor *md_accessor);

	// returns true iff all predicates in the given array are compatible with the given index
	static BOOL FCompatiblePredicates(
		gpos::pointer<CExpressionArray *> pdrgpexprPred,
		gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CColRefArray *> pdrgpcrIndex, CMDAccessor *md_accessor);

	// check if the expensive CNF conversion is beneficial in finding predicate for hash join
	static BOOL FConvertToCNF(gpos::pointer<CExpression *> pexpr,
							  gpos::pointer<CExpression *> pexprOuter,
							  gpos::pointer<CExpression *> pexprInner);

	// check if the predicate is a simple scalar cmp or a simple conjuct that can be used directly
	// for bitmap index looup without breaking it down.
	static BOOL FBitmapLookupSupportedPredicateOrConjunct(
		gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefSet *> outer_refs);

	// Check if a given comparison operator is "very strict", meaning that it is strict
	// (NULL operands result in NULL result) and that it never produces a NULL result on
	// non-null operands. Note that this check may return false negatives (some operators
	// may have very strict behavior, yet this method returns false).
	static BOOL FBuiltInComparisonIsVeryStrict(gpos::pointer<IMDId *> mdid);

	// Check if the given expr only contains conjuncts of strict comparison operators
	// NB: This does NOT recurse into Boolean AND/OR operations
	static BOOL ExprContainsOnlyStrictComparisons(
		CMemoryPool *mp, gpos::pointer<CExpression *> expr);

	// Check if the given exprs only contains conjuncts of strict comparison operators
	// NB: This does NOT recurse into Boolean AND/OR operations
	static BOOL ExprsContainsOnlyStrictComparisons(
		gpos::pointer<CExpressionArray *> conjuncts);

};	// class CPredicateUtils
}  // namespace gpopt


#endif	// !GPOPT_CPredicateUtils_H

// EOF
