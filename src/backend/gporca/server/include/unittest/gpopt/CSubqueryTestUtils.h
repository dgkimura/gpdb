//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CSubqueryTestUtils.h
//
//	@doc:
//		Optimizer test utility functions for tests requiring subquery
//		expressions
//---------------------------------------------------------------------------
#ifndef GPOPT_CSubqueryTestUtils_H
#define GPOPT_CSubqueryTestUtils_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "unittest/gpopt/CTestUtils.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CSubqueryTestUtils
//
//	@doc:
//		Test utility functions for tests requiring subquery expressions
//
//---------------------------------------------------------------------------
class CSubqueryTestUtils
{
public:
	//-------------------------------------------------------------------
	// Helpers for generating expressions
	//-------------------------------------------------------------------

	// helper to generate a pair of random Get expressions
	static void GenerateGetExpressions(CMemoryPool *mp,
									   gpos::Ref<CExpression> *ppexprOuter,
									   gpos::Ref<CExpression> *ppexprInner);

	// generate a random join expression with a subquery predicate
	static gpos::Ref<CExpression> PexprJoinWithAggSubquery(CMemoryPool *mp,
														   BOOL fCorrelated);

	// generate a select  expression with subquery equality predicate
	static gpos::Ref<CExpression> PexprSelectWithAggSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate a Select expression with a subquery equality predicate involving constant
	static gpos::Ref<CExpression> PexprSelectWithAggSubqueryConstComparison(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate a project  expression with subquery equality predicate
	static gpos::Ref<CExpression> PexprProjectWithAggSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate a random select expression with a subquery predicate
	static gpos::Ref<CExpression> PexprSelectWithAggSubquery(CMemoryPool *mp,
															 BOOL fCorrelated);

	// generate a random select expression with a subquery predicate involving constant
	static gpos::Ref<CExpression> PexprSelectWithAggSubqueryConstComparison(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with a subquery in project element
	static gpos::Ref<CExpression> PexprProjectWithAggSubquery(CMemoryPool *mp,
															  BOOL fCorrelated);

	// generate a random select expression with a subquery over join predicate
	static gpos::Ref<CExpression> PexprSelectWithAggSubqueryOverJoin(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with ANY subquery predicate
	static gpos::Ref<CExpression> PexprSelectWithAnySubquery(CMemoryPool *mp,
															 BOOL fCorrelated);

	// generate a random select expression with ANY subquery predicate over window operation
	static gpos::Ref<CExpression> PexprSelectWithAnySubqueryOverWindow(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with Any subquery whose inner expression is a GbAgg
	static gpos::Ref<CExpression> PexprSelectWithAnyAggSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with ANY subquery
	static gpos::Ref<CExpression> PexprProjectWithAnySubquery(CMemoryPool *mp,
															  BOOL fCorrelated);

	// generate a random select expression with ALL subquery predicate
	static gpos::Ref<CExpression> PexprSelectWithAllSubquery(CMemoryPool *mp,
															 BOOL fCorrelated);

	// generate a random select expression with ALL subquery predicate over window operation
	static gpos::Ref<CExpression> PexprSelectWithAllSubqueryOverWindow(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with Any subquery whose inner expression is a GbAgg
	static gpos::Ref<CExpression> PexprSelectWithAllAggSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with ALL subquery
	static gpos::Ref<CExpression> PexprProjectWithAllSubquery(CMemoryPool *mp,
															  BOOL fCorrelated);

	// generate a random select expression with EXISTS subquery predicate
	static gpos::Ref<CExpression> PexprSelectWithExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with trimmable EXISTS subquery predicate
	static gpos::Ref<CExpression> PexprSelectWithTrimmableExists(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with trimmable NOT EXISTS subquery predicate
	static gpos::Ref<CExpression> PexprSelectWithTrimmableNotExists(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with NOT EXISTS subquery predicate
	static gpos::Ref<CExpression> PexprSelectWithNotExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with EXISTS subquery
	static gpos::Ref<CExpression> PexprProjectWithExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with NOT EXISTS subquery
	static gpos::Ref<CExpression> PexprProjectWithNotExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested comparisons involving subqueries
	static gpos::Ref<CExpression> PexprSelectWithNestedCmpSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate randomized select expression with comparison between two subqueries
	static gpos::Ref<CExpression> PexprSelectWithCmpSubqueries(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested agg subqueries
	static gpos::Ref<CExpression> PexprSelectWithNestedSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested All subqueries
	static gpos::Ref<CExpression> PexprSelectWithNestedAllSubqueries(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested Any subqueries
	static gpos::Ref<CExpression> PexprSelectWithNestedAnySubqueries(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with 2-levels correlated subqueries
	static gpos::Ref<CExpression> PexprSelectWith2LevelsCorrSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with subquery predicates in an AND tree
	static gpos::Ref<CExpression> PexprSelectWithSubqueryConjuncts(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with subquery predicates in an OR tree
	static gpos::Ref<CExpression> PexprSelectWithSubqueryDisjuncts(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with subqueries
	static gpos::Ref<CExpression> PexprProjectWithSubqueries(CMemoryPool *mp,
															 BOOL fCorrelated);

	// generate a randomized Select expression to be used for building subquery examples
	static gpos::Ref<CExpression> PexprSubquery(
		CMemoryPool *mp, CExpression *pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate a quantified subquery expression
	static gpos::Ref<CExpression> PexprSubqueryQuantified(
		CMemoryPool *mp, COperator::EOperatorId op_id, CExpression *pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate quantified subquery expression over window operations
	static gpos::Ref<CExpression> PexprSelectWithSubqueryQuantifiedOverWindow(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate existential subquery expression
	static gpos::Ref<CExpression> PexprSubqueryExistential(
		CMemoryPool *mp, COperator::EOperatorId op_id, CExpression *pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate a ScalarSubquery aggregate expression
	static gpos::Ref<CExpression> PexprSubqueryAgg(
		CMemoryPool *mp, CExpression *pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate a random select expression with nested quantified subqueries
	static gpos::Ref<CExpression> PexprSelectWithNestedQuantifiedSubqueries(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized quantified subquery expression
	static gpos::Ref<CExpression> PexprSelectWithSubqueryQuantified(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized quantified subquery expression whose inner expression is a Gb
	static gpos::Ref<CExpression> PexprSelectWithQuantifiedAggSubquery(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized Project expression with quantified subquery
	static gpos::Ref<CExpression> PexprProjectWithSubqueryQuantified(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, COperator::EOperatorId op_id,
		BOOL fCorrelated);

	// generate randomized Select with existential subquery expression
	static gpos::Ref<CExpression> PexprSelectWithSubqueryExistential(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized Select expression with existential subquery predicate that can be trimmed
	static gpos::Ref<CExpression> PexprSelectWithTrimmableExistentialSubquery(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized Project with existential subquery expression
	static gpos::Ref<CExpression> PexprProjectWithSubqueryExistential(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate a Select expression with an AND predicate tree involving subqueries
	static gpos::Ref<CExpression> PexprSelectWithSubqueryBoolOp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated,
		CScalarBoolOp::EBoolOperator);

	// generate a Project expression with multiple subqueries
	static gpos::Ref<CExpression> PexprProjectWithSubqueries(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fCorrelated);

	// generate a Select expression with ANY/ALL subquery predicate over a const table get
	static gpos::Ref<CExpression> PexprSubqueryWithConstTableGet(
		CMemoryPool *mp, COperator::EOperatorId op_id);

	// generate a Select expression with a disjunction of two ANY subqueries
	static gpos::Ref<CExpression> PexprSubqueryWithDisjunction(CMemoryPool *mp);

	// generate an expression with subquery in null test context
	static gpos::Ref<CExpression> PexprSubqueriesInNullTestContext(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with subqueries in both value and filter contexts
	static gpos::Ref<CExpression> PexprSubqueriesInDifferentContexts(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable quantified subquery
	static gpos::Ref<CExpression> PexprUndecorrelatableSubquery(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate an expression with undecorrelatable ANY subquery
	static gpos::Ref<CExpression> PexprUndecorrelatableAnySubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable ALL subquery
	static gpos::Ref<CExpression> PexprUndecorrelatableAllSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable exists subquery
	static gpos::Ref<CExpression> PexprUndecorrelatableExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable NotExists subquery
	static gpos::Ref<CExpression> PexprUndecorrelatableNotExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable Scalar subquery
	static gpos::Ref<CExpression> PexprUndecorrelatableScalarSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

};	// class CSubqueryTestUtils

}  // namespace gpopt

#endif	// !GPOPT_CSubqueryTestUtils_H

// EOF
