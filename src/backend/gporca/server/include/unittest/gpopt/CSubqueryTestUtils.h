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
									   gpos::owner<CExpression *> *ppexprOuter,
									   gpos::owner<CExpression *> *ppexprInner);

	// generate a random join expression with a subquery predicate
	static gpos::owner<CExpression *> PexprJoinWithAggSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a select  expression with subquery equality predicate
	static gpos::owner<CExpression *> PexprSelectWithAggSubquery(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate a Select expression with a subquery equality predicate involving constant
	static gpos::owner<CExpression *> PexprSelectWithAggSubqueryConstComparison(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate a project  expression with subquery equality predicate
	static gpos::owner<CExpression *> PexprProjectWithAggSubquery(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate a random select expression with a subquery predicate
	static gpos::owner<CExpression *> PexprSelectWithAggSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with a subquery predicate involving constant
	static gpos::owner<CExpression *> PexprSelectWithAggSubqueryConstComparison(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with a subquery in project element
	static gpos::owner<CExpression *> PexprProjectWithAggSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with a subquery over join predicate
	static gpos::owner<CExpression *> PexprSelectWithAggSubqueryOverJoin(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with ANY subquery predicate
	static gpos::owner<CExpression *> PexprSelectWithAnySubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with ANY subquery predicate over window operation
	static gpos::owner<CExpression *> PexprSelectWithAnySubqueryOverWindow(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with Any subquery whose inner expression is a GbAgg
	static gpos::owner<CExpression *> PexprSelectWithAnyAggSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with ANY subquery
	static gpos::owner<CExpression *> PexprProjectWithAnySubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with ALL subquery predicate
	static gpos::owner<CExpression *> PexprSelectWithAllSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with ALL subquery predicate over window operation
	static gpos::owner<CExpression *> PexprSelectWithAllSubqueryOverWindow(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with Any subquery whose inner expression is a GbAgg
	static gpos::owner<CExpression *> PexprSelectWithAllAggSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with ALL subquery
	static gpos::owner<CExpression *> PexprProjectWithAllSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with EXISTS subquery predicate
	static gpos::owner<CExpression *> PexprSelectWithExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with trimmable EXISTS subquery predicate
	static gpos::owner<CExpression *> PexprSelectWithTrimmableExists(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with trimmable NOT EXISTS subquery predicate
	static gpos::owner<CExpression *> PexprSelectWithTrimmableNotExists(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with NOT EXISTS subquery predicate
	static gpos::owner<CExpression *> PexprSelectWithNotExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with EXISTS subquery
	static gpos::owner<CExpression *> PexprProjectWithExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with NOT EXISTS subquery
	static gpos::owner<CExpression *> PexprProjectWithNotExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested comparisons involving subqueries
	static gpos::owner<CExpression *> PexprSelectWithNestedCmpSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate randomized select expression with comparison between two subqueries
	static gpos::owner<CExpression *> PexprSelectWithCmpSubqueries(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested agg subqueries
	static gpos::owner<CExpression *> PexprSelectWithNestedSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested All subqueries
	static gpos::owner<CExpression *> PexprSelectWithNestedAllSubqueries(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with nested Any subqueries
	static gpos::owner<CExpression *> PexprSelectWithNestedAnySubqueries(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with 2-levels correlated subqueries
	static gpos::owner<CExpression *> PexprSelectWith2LevelsCorrSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with subquery predicates in an AND tree
	static gpos::owner<CExpression *> PexprSelectWithSubqueryConjuncts(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random select expression with subquery predicates in an OR tree
	static gpos::owner<CExpression *> PexprSelectWithSubqueryDisjuncts(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a random project expression with subqueries
	static gpos::owner<CExpression *> PexprProjectWithSubqueries(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate a randomized Select expression to be used for building subquery examples
	static gpos::owner<CExpression *> PexprSubquery(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate a quantified subquery expression
	static gpos::owner<CExpression *> PexprSubqueryQuantified(
		CMemoryPool *mp, COperator::EOperatorId op_id,
		gpos::pointer<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate quantified subquery expression over window operations
	static gpos::owner<CExpression *>
	PexprSelectWithSubqueryQuantifiedOverWindow(CMemoryPool *mp,
												COperator::EOperatorId op_id,
												BOOL fCorrelated);

	// generate existential subquery expression
	static gpos::owner<CExpression *> PexprSubqueryExistential(
		CMemoryPool *mp, COperator::EOperatorId op_id,
		gpos::pointer<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate a ScalarSubquery aggregate expression
	static gpos::owner<CExpression *> PexprSubqueryAgg(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate a random select expression with nested quantified subqueries
	static gpos::owner<CExpression *> PexprSelectWithNestedQuantifiedSubqueries(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized quantified subquery expression
	static gpos::owner<CExpression *> PexprSelectWithSubqueryQuantified(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized quantified subquery expression whose inner expression is a Gb
	static gpos::owner<CExpression *> PexprSelectWithQuantifiedAggSubquery(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized Project expression with quantified subquery
	static gpos::owner<CExpression *> PexprProjectWithSubqueryQuantified(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, COperator::EOperatorId op_id,
		BOOL fCorrelated);

	// generate randomized Select with existential subquery expression
	static gpos::owner<CExpression *> PexprSelectWithSubqueryExistential(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate randomized Select expression with existential subquery predicate that can be trimmed
	static gpos::owner<CExpression *>
	PexprSelectWithTrimmableExistentialSubquery(CMemoryPool *mp,
												COperator::EOperatorId op_id,
												BOOL fCorrelated);

	// generate randomized Project with existential subquery expression
	static gpos::owner<CExpression *> PexprProjectWithSubqueryExistential(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate a Select expression with an AND predicate tree involving subqueries
	static gpos::owner<CExpression *> PexprSelectWithSubqueryBoolOp(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated,
		CScalarBoolOp::EBoolOperator);

	// generate a Project expression with multiple subqueries
	static gpos::owner<CExpression *> PexprProjectWithSubqueries(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::owner<CExpression *> pexprInner, BOOL fCorrelated);

	// generate a Select expression with ANY/ALL subquery predicate over a const table get
	static gpos::owner<CExpression *> PexprSubqueryWithConstTableGet(
		CMemoryPool *mp, COperator::EOperatorId op_id);

	// generate a Select expression with a disjunction of two ANY subqueries
	static gpos::owner<CExpression *> PexprSubqueryWithDisjunction(
		CMemoryPool *mp);

	// generate an expression with subquery in null test context
	static gpos::owner<CExpression *> PexprSubqueriesInNullTestContext(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with subqueries in both value and filter contexts
	static gpos::owner<CExpression *> PexprSubqueriesInDifferentContexts(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable quantified subquery
	static gpos::owner<CExpression *> PexprUndecorrelatableSubquery(
		CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated);

	// generate an expression with undecorrelatable ANY subquery
	static gpos::owner<CExpression *> PexprUndecorrelatableAnySubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable ALL subquery
	static gpos::owner<CExpression *> PexprUndecorrelatableAllSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable exists subquery
	static gpos::owner<CExpression *> PexprUndecorrelatableExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable NotExists subquery
	static gpos::owner<CExpression *> PexprUndecorrelatableNotExistsSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

	// generate an expression with undecorrelatable Scalar subquery
	static gpos::owner<CExpression *> PexprUndecorrelatableScalarSubquery(
		CMemoryPool *mp, BOOL fCorrelated);

};	// class CSubqueryTestUtils

}  // namespace gpopt

#endif	// !GPOPT_CSubqueryTestUtils_H

// EOF
