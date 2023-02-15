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
	static gpos::Ref<CExpression> PexprConjEqualityPredicates(
		CMemoryPool *mp, gpos::Ref<CColRefSet> pcrs);

	// additional equality predicates are generated based on the equivalence
	// classes in the constraint properties of the expression
	static gpos::Ref<CExpression> PexprAddEqualityPreds(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSet> pcrsProcessed);

	// check if all columns in the given equivalence class come from one of the
	// children of the given expression
	static BOOL FEquivClassFromChild(gpos::Ref<CColRefSet> pcrs,
									 gpos::Ref<CExpression> pexpr);

	// generate predicates for the given set of columns based on the given
	// constraint property
	static gpos::Ref<CExpression> PexprScalarPredicates(
		CMemoryPool *mp, gpos::Ref<CPropConstraint> ppc,
		gpos::Ref<CPropConstraint> constraintsForOuterRefs,
		gpos::Ref<CColRefSet> pcrsNotNull, gpos::Ref<CColRefSet> pcrs,
		gpos::Ref<CColRefSet> pcrsProcessed);

	// eliminate self comparisons
	static gpos::Ref<CExpression> PexprEliminateSelfComparison(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// remove CTE Anchor nodes
	static gpos::Ref<CExpression> PexprRemoveCTEAnchors(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// trim superfluos equality
	static gpos::Ref<CExpression> PexprPruneSuperfluousEquality(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// trim existential subqueries
	static gpos::Ref<CExpression> PexprTrimExistentialSubqueries(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// simplify quantified subqueries
	static gpos::Ref<CExpression> PexprSimplifyQuantifiedSubqueries(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// preliminary unnesting of scalar  subqueries
	static gpos::Ref<CExpression> PexprUnnestScalarSubqueries(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// remove superfluous limit nodes
	static gpos::Ref<CExpression> PexprRemoveSuperfluousLimit(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// remove superfluous distinct nodes
	static gpos::Ref<CExpression> PexprRemoveSuperfluousDistinctInDQA(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// remove superfluous outer references from limit, group by and window operators
	static gpos::Ref<CExpression> PexprRemoveSuperfluousOuterRefs(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// generate predicates based on derived constraint properties
	static gpos::Ref<CExpression> PexprFromConstraints(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSet> pcrsProcessed,
		gpos::Ref<CPropConstraint> constraintsForOuterRefs);

	// generate predicates based on derived constraint properties under scalar expressions
	static gpos::Ref<CExpression> PexprFromConstraintsScalar(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CPropConstraint> constraintsForOuterRefs);

	// eliminate subtrees that have zero output cardinality
	static gpos::Ref<CExpression> PexprPruneEmptySubtrees(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// collapse cascaded inner joins into NAry-joins
	static gpos::Ref<CExpression> PexprCollapseJoins(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// helper method for PexprCollapseJoins, collect children and make recursive calls
	static void CollectJoinChildrenRecursively(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpressionArray> logicalLeafNodes,
		gpos::Ref<ULongPtrArray> lojChildPredIndexes,
		gpos::Ref<CExpressionArray> innerJoinPredicates,
		gpos::Ref<CExpressionArray> lojPredicates);

	// collapse cascaded logical project operators
	static gpos::Ref<CExpression> PexprCollapseProjects(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// add dummy project element below scalar subquery when the output column is an outer reference
	static gpos::Ref<CExpression> PexprProjBelowSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL fUnderPrList);

	// helper function to rewrite IN query to simple EXISTS with a predicate
	static gpos::Ref<CExpression> ConvertInToSimpleExists(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// rewrite IN subquery to EXIST subquery with a predicate
	static gpos::Ref<CExpression> PexprExistWithPredFromINSubq(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// collapse cascaded union/union all into an NAry union/union all operator
	static gpos::Ref<CExpression> PexprCollapseUnionUnionAll(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// transform outer joins into inner joins whenever possible
	static gpos::Ref<CExpression> PexprOuterJoinToInnerJoin(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// eliminate CTE Anchors for CTEs that have zero consumers
	static gpos::Ref<CExpression> PexprRemoveUnusedCTEs(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// collect CTE predicates from consumers
	static void CollectCTEPredicates(CMemoryPool *mp,
									 gpos::Ref<CExpression> pexpr,
									 CTEPredsMap *phm);

	// imply new predicates on LOJ's inner child based on constraints derived from LOJ's outer child and join predicate
	static gpos::Ref<CExpression> PexprWithImpliedPredsOnLOJInnerChild(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLOJ,
		BOOL *pfAddedPredicates);

	// infer predicate from outer child to inner child of the outer join
	static gpos::Ref<CExpression>
	PexprOuterJoinInferPredsFromOuterChildToInnerChild(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL *pfAddedPredicates);

	// driver for inferring predicates from constraints
	static gpos::Ref<CExpression> PexprInferPredicates(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// entry for pruning unused computed columns
	static gpos::Ref<CExpression> PexprPruneUnusedComputedCols(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSet> pcrsReqd);

	// driver for pruning unused computed columns
	static gpos::Ref<CExpression> PexprPruneUnusedComputedColsRecursive(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSet> pcrsReqd);

	// prune unused project elements from the project list of Project or GbAgg
	static gpos::Ref<CExpression> PexprPruneProjListProjectOrGbAgg(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSet> pcrsUnused, gpos::Ref<CColRefSet> pcrsDefined,
		const gpos::Ref<CColRefSet> pcrsReqd);

	// generate a scalar bool op expression or return the only child expression in array
	static gpos::Ref<CExpression> PexprScalarBoolOpConvert2In(
		CMemoryPool *mp, CScalarBoolOp::EBoolOperator eboolop,
		gpos::Ref<CExpressionArray> pdrgpexpr);

	// determines if the expression is likely convertible to an array expression
	static BOOL FConvert2InIsConvertable(gpos::Ref<CExpression> pexpr,
										 CScalarBoolOp::EBoolOperator eboolop);

	// reorder the scalar cmp children to ensure that left child is Scalar Ident and right Child is Scalar Const
	static gpos::Ref<CExpression> PexprReorderScalarCmpChildren(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	static gpos::Ref<CExpression> PrunePartitions(CMemoryPool *mp,
												  gpos::Ref<CExpression> expr);

	static gpos::Ref<CConstraint> PcnstrFromChildPartition(
		const IMDRelation *partrel, CColRefArray *pdrgpcrOutput,
		ColRefToUlongMap *col_mapping);

	// swap logical select over logical project
	static gpos::Ref<CExpression> PexprTransposeSelectAndProject(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	static gpos::Ref<CExpression> ConvertSplitUpdateToInPlaceUpdate(
		CMemoryPool *mp, gpos::Ref<CExpression> expr);

	// private ctor
	CExpressionPreprocessor();

	// private dtor
	virtual ~CExpressionPreprocessor();

	// private copy ctor
	CExpressionPreprocessor(const CExpressionPreprocessor &);

public:
	// main driver
	static gpos::Ref<CExpression> PexprPreprocess(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSet> pcrsOutputAndOrderCols = nullptr);

	// add predicates collected from CTE consumers to producer expressions
	static void AddPredsToCTEProducers(CMemoryPool *mp,
									   gpos::Ref<CExpression> pexpr);

	// derive constraints on given expression
	static gpos::Ref<CExpression> PexprAddPredicatesFromConstraints(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// convert series of AND or OR comparisons into array IN expressions
	static gpos::Ref<CExpression> PexprConvert2In(CMemoryPool *mp,
												  gpos::Ref<CExpression> pexpr);

};	// class CExpressionPreprocessor
}  // namespace gpopt


#endif	// !GPOPT_CExpressionPreprocessor_H

// EOF
