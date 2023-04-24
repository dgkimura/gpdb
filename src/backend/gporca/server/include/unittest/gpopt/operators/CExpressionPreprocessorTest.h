//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CExpressionPreprocessorTest.h
//
//	@doc:
//		Test for expression preprocessing
//---------------------------------------------------------------------------
#ifndef GPOPT_CExpressionPreprocessorTest_H
#define GPOPT_CExpressionPreprocessorTest_H

#include "gpos/base.h"

#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CScalarBoolOp.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CExpressionPreprocessorTest
//
//	@doc:
//		Unittests
//
//---------------------------------------------------------------------------
class CExpressionPreprocessorTest
{
private:
	// shorthand for functions for generating the expression for unnest test
	using FnPexprUnnestTestCase =
		gpos::Ref<CExpression>(CMemoryPool *, gpos::Ref<CExpression>);

	// unnest scalar subqueries test cases
	struct SUnnestSubqueriesTestCase
	{
		// generator function of first expression
		FnPexprUnnestTestCase *m_pfFst;

		// generator function of second expression
		FnPexprUnnestTestCase *m_pfSnd;

		// AND/OR boolean operator
		CScalarBoolOp::EBoolOperator m_eboolop;

		// operator that should be present after unnesting
		COperator::EOperatorId m_eopidPresent;

		// negate the children of or/and
		BOOL m_fNegateChildren;
	};	// SUnnestSubqueriesTestCase

	// count number of scalar subqueries
	static ULONG UlScalarSubqs(gpos::Ref<CExpression> pexpr);

	// check if a given expression has a subquery exists node
	static BOOL FHasSubqueryExists(gpos::Ref<CExpression> pexpr);

	// check if a given expression has a subquery not exitst node
	static BOOL FHasSubqueryNotExists(gpos::Ref<CExpression> pexpr);

	// check if a given expression has an ALL subquery
	static BOOL FHasSubqueryAll(gpos::Ref<CExpression> pexpr);

	// check if a given expression has an ANY subquery
	static BOOL FHasSubqueryAny(gpos::Ref<CExpression> pexpr);

	// check the type of the subquery
	static GPOS_RESULT EresCheckSubqueryType(
		gpos::Ref<CExpression> pexpr, COperator::EOperatorId eopidPresent);

	static
		// check the type of the existential subquery
		GPOS_RESULT
		EresCheckExistsSubqueryType(gpos::Ref<CExpression> pexpr,
									COperator::EOperatorId eopidPresent);

	// check the type of the quantified subquery
	static GPOS_RESULT EresCheckQuantifiedSubqueryType(
		gpos::Ref<CExpression> pexpr, COperator::EOperatorId eopidPresent);

#ifdef GPOS_DEBUG
	// check if a given expression has no Outer Join nodes
	static BOOL FHasNoOuterJoin(gpos::Ref<CExpression> pexpr);

	// check if a given expression has outer references in any node
	static BOOL HasOuterRefs(gpos::Ref<CExpression> pexpr);

	// check if a given expression has Sequence Project nodes
	static BOOL FHasSeqPrj(gpos::Ref<CExpression> pexpr);

	// check if a given expression has IS DISTINCT FROM nodes
	static BOOL FHasIDF(gpos::Ref<CExpression> pexpr);

#endif	// GPOS_DEBUG

	// create a conjunction of comparisons using the given columns
	static gpos::Ref<CExpression> PexprCreateConjunction(
		CMemoryPool *mp, CColRefArray *colref_array);

	// Helper for preprocessing window functions with outer references
	static void PreprocessWinFuncWithOuterRefs(const CHAR *szFilePath,
											   BOOL fAllowWinFuncOuterRefs);

	// Helper for preprocessing window functions with distinct aggs
	static void PreprocessWinFuncWithDistinctAggs(CMemoryPool *mp,
												  const CHAR *szFilePath,
												  BOOL fAllowSeqPrj,
												  BOOL fAllowIDF);

	// Helper for preprocessing outer joins
	static void PreprocessOuterJoin(const CHAR *szFilePath,
									BOOL fAllowOuterJoin);

	// helper function for testing collapse of Inner Joins
	static GPOS_RESULT EresUnittest_CollapseInnerJoinHelper(
		CMemoryPool *mp, gpos::Ref<COperator> popJoin,
		gpos::Ref<CExpression> rgpexpr[],
		gpos::Ref<CDrvdPropRelational> rgpdprel[]);

	// helper function for testing cascaded inner/outer joins
	static gpos::Ref<CExpression> PexprJoinHelper(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLOJ, BOOL fCascadedLOJ,
		BOOL fIntermediateInnerjoin);

	// helper function for testing window functions with outer join
	static gpos::Ref<CExpression> PexprWindowFuncWithLOJHelper(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLOJ,
		gpos::Ref<CColRef> pcrPartitionBy, BOOL fAddWindowFunction,
		BOOL fOuterChildPred, BOOL fCascadedLOJ, BOOL fPredBelowWindow);

	// helper function for testing Select with outer join
	static gpos::Ref<CExpression> PexprSelectWithLOJHelper(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLOJ,
		gpos::Ref<CColRef> colref, BOOL fOuterChildPred, BOOL fCascadedLOJ);

	// helper function for comparing expressions resulting from preprocessing window functions with outer join
	static GPOS_RESULT EresCompareExpressions(CMemoryPool *mp,
											  CWStringDynamic *rgstr[],
											  ULONG size);

	// test case generator for outer joins
	static GPOS_RESULT EresTestLOJ(BOOL fAddWindowFunction);

	// helper to create an expression with a predicate containing an array and other comparisons
	static gpos::Ref<CExpression> PexprCreateConvertableArray(
		CMemoryPool *mp, BOOL fCreateInStatement);

public:
	// unittests
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_PreProcess();
	static GPOS_RESULT EresUnittest_InferPredsOnLOJ();
	static GPOS_RESULT EresUnittest_PreProcessWindowFunc();
	static GPOS_RESULT EresUnittest_PreProcessWindowFuncWithLOJ();
	static GPOS_RESULT EresUnittest_PreProcessWindowFuncWithOuterRefs();
	static GPOS_RESULT EresUnittest_PreProcessWindowFuncWithDistinctAggs();
	static GPOS_RESULT EresUnittest_PreProcessNestedScalarSubqueries();
	static GPOS_RESULT EresUnittest_UnnestSubqueries();
	static GPOS_RESULT EresUnittest_PreProcessOuterJoin();
	static GPOS_RESULT EresUnittest_PreProcessOuterJoinMinidumps();
	static GPOS_RESULT EresUnittest_PreProcessOrPrefilters();
	static GPOS_RESULT EresUnittest_PreProcessOrPrefiltersPartialPush();
	static GPOS_RESULT EresUnittest_CollapseInnerJoin();
	static GPOS_RESULT EresUnittest_PreProcessConvert2InPredicate();
	static GPOS_RESULT
	EresUnittest_PreProcessConvert2InPredicateDeepExpressionTree();
	static GPOS_RESULT EresUnittest_PreProcessConvertArrayWithEquals();

};	// class CExpressionPreprocessorTest
}  // namespace gpopt

#endif	// !GPOPT_CExpressionPreprocessorTest_H

// EOF
