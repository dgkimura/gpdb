//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CExpressionFactorizer.h
//
//	@doc:
//		Utility functions for expression factorization.
//
//	@owner:
//		,
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPOPT_CExpressionFactorizer_H
#define GPOPT_CExpressionFactorizer_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/COperator.h"

namespace gpopt
{
using namespace gpos;

// fwd declarations
class ExprMap;

//---------------------------------------------------------------------------
//	@class:
//		CExpressionFactorizer
//
//	@doc:
//		Expression factorization routines.
//
//---------------------------------------------------------------------------
class CExpressionFactorizer
{
private:
	// map expression to a count, used in factorization
	using ExprMap =
		CHashMap<CExpression, ULONG, CExpression::HashValue, CUtils::Equals,
				 CleanupRelease<CExpression>, CleanupDelete<ULONG>>;

	// map operators to an array of expression arrays, corresponding to
	// a disjunction of expressions on columns created by that operator
	using SourceToArrayPosMap =
		CHashMap<ULONG, CExpressionArrays, gpos::HashValue<ULONG>,
				 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
				 CleanupRelease<CExpressionArrays>>;

	// iterator for map of operator to disjunctive form representation
	using SourceToArrayPosMapIter =
		CHashMapIter<ULONG, CExpressionArrays, gpos::HashValue<ULONG>,
					 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
					 CleanupRelease<CExpressionArrays>>;

	// map columns to an array of expression arrays, corresponding to
	// a disjunction of expressions using that column
	using ColumnToArrayPosMap =
		gpos::UnorderedMap<const CColRef *, gpos::Ref<CExpressionArrays>,
						   gpos::PtrHash<CColRef, gpos::HashPtr<CColRef>>,
						   gpos::PtrEqual<CColRef, gpos::EqualPtr<CColRef>>>;

	// iterator for map of column to disjunctive form representation
	using ColumnToArrayPosMapIter = gpos::UnorderedMap<
		const CColRef *, gpos::Ref<CExpressionArrays>,
		gpos::PtrHash<CColRef, gpos::HashPtr<CColRef>>,
		gpos::PtrEqual<CColRef, gpos::EqualPtr<CColRef>>>::LegacyIterator;

	using PexprProcessDisj = gpos::Ref<CExpression> (*)(CMemoryPool *,
														gpos::Ref<CExpression>,
														gpos::Ref<CExpression>);

	// helper for determining if given expression factor should be added to factors array
	static void AddFactor(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
						  CExpressionArray *pdrgpexprFactors,
						  CExpressionArray *pdrgpexprResidual,
						  ExprMap *pexprmap, ULONG ulDisjuncts);

	// helper for building a factors map
	static gpos::Ref<ExprMap> PexprmapFactors(CMemoryPool *mp,
											  gpos::Ref<CExpression> pexpr);

	// factorize common expressions in Or tree
	static gpos::Ref<CExpression> PexprFactorizeDisj(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression>	// pexprLowestLogicalAncestor
	);

	// visitor-like function to process descendents that are OR operators
	static gpos::Ref<CExpression> PexprProcessDisjDescendents(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprLowestLogicalAncestor,
		PexprProcessDisj pexprorfun);

	// discover common factors in scalar expression
	static gpos::Ref<CExpression> PexprDiscoverFactors(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// if the given expression is a non volatile scalar expression using table
	// columns created by the same operator
	// return true and set ulOpSourceId to the id of the operator that created
	// that column,
	// or if the given expression is a non volatile scalar expression using
	// only one computed column
	// return true and set ppcrComputedColumn to that computed column,
	// otherwise return false
	static BOOL FOpSourceIdOrComputedColumn(gpos::Ref<CExpression> pexpr,
											ULONG *ulOpSourceId,
											CColRef **ppcrComputedColumn);

	// if the given expression is a scalar that can be pushed, it returns
	// the set of used columns
	static gpos::Ref<CColRefSet> PcrsUsedByPushableScalar(
		gpos::Ref<CExpression> pexpr);

	// find the array of expression arrays corresponding to the given
	// operator source id in the given source to array position map
	// or construct a new array and add it to the map
	static CExpressionArrays *PdrgPdrgpexprDisjunctArrayForSourceId(
		CMemoryPool *mp, SourceToArrayPosMap *psrc2array, BOOL fAllowNewSources,
		ULONG ulOpSourceId);

	// find the array of expression arrays corresponding to the given
	// column in the given column to array position map
	// or construct a new array and add it to the map
	static CExpressionArrays *PdrgPdrgpexprDisjunctArrayForColumn(
		CMemoryPool *mp, ColumnToArrayPosMap *pcol2array, BOOL fAllowNewSources,
		CColRef *colref);

	// if the given expression is a table column to constant comparison,
	// record it in the map
	static void StoreBaseOpToColumnExpr(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		SourceToArrayPosMap *psrc2array, ColumnToArrayPosMap *pcol2array,
		const gpos::Ref<CColRefSet> pcrsProducedByChildren,
		BOOL fAllowNewSources, ULONG ulPosition);

	// construct a filter based on the expressions from 'pdrgpdrgpexpr'
	// and add to the array 'pdrgpexprPrefilters'
	static void AddInferredFiltersFromArray(
		CMemoryPool *mp, const CExpressionArrays *pdrgpdrgpexpr,
		ULONG ulDisjChildrenLength, CExpressionArray *pdrgpexprPrefilters);

	// create a conjunction of the given expression and inferred filters constructed out
	// of the given maps
	static gpos::Ref<CExpression> PexprAddInferredFilters(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		SourceToArrayPosMap *psrc2array, ColumnToArrayPosMap *pcol2array);

	//	returns the set of columns produced by the scalar children of the given
	//	expression
	static gpos::Ref<CColRefSet> PcrsColumnsProducedByChildren(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// compute disjunctive pre-filters that can be pushed to the column creators
	static gpos::Ref<CExpression> PexprExtractInferredFiltersFromDisj(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprLowestLogicalAncestor);

public:
	// factorize common expressions
	static gpos::Ref<CExpression> PexprFactorize(CMemoryPool *mp,
												 gpos::Ref<CExpression> pexpr);

	// compute disjunctive pre-filters that can be pushed to the column creators
	static gpos::Ref<CExpression> PexprExtractInferredFilters(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);
};
}  // namespace gpopt

#endif	// ! GPOPT_CExpressionFactorizer_H

// EOF
