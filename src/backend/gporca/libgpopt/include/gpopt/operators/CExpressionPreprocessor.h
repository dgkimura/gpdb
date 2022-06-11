//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2012 EMC Corp.
//
//	@filename:
//		CExpressionPreprocessor.h
//
//	@doc:
//		Expression tree preprocessing routines, needed to prepare an input
//		logical expression to be optimized
//
//---------------------------------------------------------------------------
#ifndef GPOPT_CExpressionPreprocessor_H
#define GPOPT_CExpressionPreprocessor_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColumnFactory.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CScalarBoolOp.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CExpressionPreprocessor
//
//	@doc:
//		Expression preprocessing routines
//
//---------------------------------------------------------------------------
class CExpressionPreprocessor
{
private:
	// map CTE id to collected predicates
	using CTEPredsMap =
		CHashMap<ULONG, CExpressionArray, gpos::HashValue<ULONG>,
				 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
				 CleanupRelease<CExpressionArray>>;

	// iterator for map of CTE id to collected predicates
	using CTEPredsMapIter =
		CHashMapIter<ULONG, CExpressionArray, gpos::HashValue<ULONG>,
					 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
					 CleanupRelease<CExpressionArray>>;

	// generate a conjunction of equality predicates between the columns in the given set
	static gpos::owner<CExpression *> PexprConjEqualityPredicates(
		CMemoryPool *mp, gpos::pointer<CColRefSet *> pcrs);

	// additional equality predicates are generated based on the equivalence
	// classes in the constraint properties of the expression
	static gpos::owner<CExpression *> PexprAddEqualityPreds(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		CColRefSet *pcrsProcessed);

	// check if all columns in the given equivalence class come from one of the
	// children of the given expression
	static BOOL FEquivClassFromChild(gpos::pointer<CColRefSet *> pcrs,
									 gpos::pointer<CExpression *> pexpr);

	// generate predicates for the given set of columns based on the given
	// constraint property
	static gpos::owner<CExpression *> PexprScalarPredicates(
		CMemoryPool *mp, gpos::pointer<CPropConstraint *> ppc,
		gpos::pointer<CPropConstraint *> constraintsForOuterRefs,
		gpos::pointer<CColRefSet *> pcrsNotNull,
		gpos::pointer<CColRefSet *> pcrs,
		gpos::pointer<CColRefSet *> pcrsProcessed);

	// eliminate self comparisons
	static gpos::owner<CExpression *> PexprEliminateSelfComparison(
		CMemoryPool *mp, CExpression *pexpr);

	// remove CTE Anchor nodes
	static CExpression *PexprRemoveCTEAnchors(CMemoryPool *mp,
											  CExpression *pexpr);

	// trim superfluos equality
	static gpos::owner<CExpression *> PexprPruneSuperfluousEquality(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// trim existential subqueries
	static gpos::owner<CExpression *> PexprTrimExistentialSubqueries(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// simplify quantified subqueries
	static gpos::owner<CExpression *> PexprSimplifyQuantifiedSubqueries(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// preliminary unnesting of scalar  subqueries
	static gpos::owner<CExpression *> PexprUnnestScalarSubqueries(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// remove superfluous limit nodes
	static gpos::owner<CExpression *> PexprRemoveSuperfluousLimit(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// remove superfluous distinct nodes
	static gpos::owner<CExpression *> PexprRemoveSuperfluousDistinctInDQA(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// remove superfluous outer references from limit, group by and window operators
	static gpos::owner<CExpression *> PexprRemoveSuperfluousOuterRefs(
		CMemoryPool *mp, CExpression *pexpr);

	// generate predicates based on derived constraint properties
	static gpos::owner<CExpression *> PexprFromConstraints(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		CColRefSet *pcrsProcessed, CPropConstraint *constraintsForOuterRefs);

	// generate predicates based on derived constraint properties under scalar expressions
	static gpos::owner<CExpression *> PexprFromConstraintsScalar(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		CPropConstraint *constraintsForOuterRefs);

	// eliminate subtrees that have zero output cardinality
	static gpos::owner<CExpression *> PexprPruneEmptySubtrees(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// collapse cascaded inner joins into NAry-joins
	static gpos::owner<CExpression *> PexprCollapseJoins(CMemoryPool *mp,
														 CExpression *pexpr);

	// helper method for PexprCollapseJoins, collect children and make recursive calls
	static void CollectJoinChildrenRecursively(
		CMemoryPool *mp, CExpression *pexpr, CExpressionArray *logicalLeafNodes,
		ULongPtrArray *lojChildPredIndexes,
		CExpressionArray *innerJoinPredicates, CExpressionArray *lojPredicates);

	// collapse cascaded logical project operators
	static gpos::owner<CExpression *> PexprCollapseProjects(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// add dummy project element below scalar subquery when the output column is an outer reference
	static gpos::owner<CExpression *> PexprProjBelowSubquery(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, BOOL fUnderPrList);

	// helper function to rewrite IN query to simple EXISTS with a predicate
	static gpos::owner<CExpression *> ConvertInToSimpleExists(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// rewrite IN subquery to EXIST subquery with a predicate
	static gpos::owner<CExpression *> PexprExistWithPredFromINSubq(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// collapse cascaded union/union all into an NAry union/union all operator
	static gpos::owner<CExpression *> PexprCollapseUnionUnionAll(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// transform outer joins into inner joins whenever possible
	static gpos::owner<CExpression *> PexprOuterJoinToInnerJoin(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// eliminate CTE Anchors for CTEs that have zero consumers
	static gpos::owner<CExpression *> PexprRemoveUnusedCTEs(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// collect CTE predicates from consumers
	static void CollectCTEPredicates(CMemoryPool *mp,
									 gpos::pointer<CExpression *> pexpr,
									 CTEPredsMap *phm);

	// imply new predicates on LOJ's inner child based on constraints derived from LOJ's outer child and join predicate
	static gpos::owner<CExpression *> PexprWithImpliedPredsOnLOJInnerChild(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprLOJ,
		BOOL *pfAddedPredicates);

	// infer predicate from outer child to inner child of the outer join
	static gpos::owner<CExpression *>
	PexprOuterJoinInferPredsFromOuterChildToInnerChild(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		BOOL *pfAddedPredicates);

	// driver for inferring predicates from constraints
	static gpos::owner<CExpression *> PexprInferPredicates(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// entry for pruning unused computed columns
	static gpos::owner<CExpression *> PexprPruneUnusedComputedCols(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefSet *> pcrsReqd);

	// driver for pruning unused computed columns
	static gpos::owner<CExpression *> PexprPruneUnusedComputedColsRecursive(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		CColRefSet *pcrsReqd);

	// prune unused project elements from the project list of Project or GbAgg
	static gpos::owner<CExpression *> PexprPruneProjListProjectOrGbAgg(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefSet *> pcrsUnused,
		gpos::pointer<CColRefSet *> pcrsDefined,
		gpos::pointer<const CColRefSet *> pcrsReqd);

	// generate a scalar bool op expression or return the only child expression in array
	static gpos::owner<CExpression *> PexprScalarBoolOpConvert2In(
		CMemoryPool *mp, CScalarBoolOp::EBoolOperator eboolop,
		gpos::owner<CExpressionArray *> pdrgpexpr);

	// determines if the expression is likely convertible to an array expression
	static BOOL FConvert2InIsConvertable(gpos::pointer<CExpression *> pexpr,
										 CScalarBoolOp::EBoolOperator eboolop);

	// reorder the scalar cmp children to ensure that left child is Scalar Ident and right Child is Scalar Const
	static gpos::owner<CExpression *> PexprReorderScalarCmpChildren(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	static gpos::owner<CExpression *> PrunePartitions(
		CMemoryPool *mp, gpos::pointer<CExpression *> expr);

	static gpos::owner<CConstraint *> PcnstrFromChildPartition(
		gpos::pointer<const IMDRelation *> partrel,
		gpos::pointer<CColRefArray *> pdrgpcrOutput,
		gpos::pointer<ColRefToUlongMap *> col_mapping);

	// private ctor
	CExpressionPreprocessor();

	// private dtor
	virtual ~CExpressionPreprocessor();

	// private copy ctor
	CExpressionPreprocessor(const CExpressionPreprocessor &);

public:
	// main driver
	static gpos::owner<CExpression *> PexprPreprocess(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefSet *> pcrsOutputAndOrderCols = nullptr);

	// add predicates collected from CTE consumers to producer expressions
	static void AddPredsToCTEProducers(CMemoryPool *mp,
									   gpos::pointer<CExpression *> pexpr);

	// derive constraints on given expression
	static gpos::owner<CExpression *> PexprAddPredicatesFromConstraints(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// convert series of AND or OR comparisons into array IN expressions
	static gpos::owner<CExpression *> PexprConvert2In(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

};	// class CExpressionPreprocessor
}  // namespace gpopt


#endif	// !GPOPT_CExpressionPreprocessor_H

// EOF
