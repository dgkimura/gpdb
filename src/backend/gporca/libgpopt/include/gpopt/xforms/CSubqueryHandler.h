//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CSubqueryHandler.h
//
//	@doc:
//		Helper class for transforming subquery expressions to Apply
//		expressions
//---------------------------------------------------------------------------
#ifndef GPOPT_CSubqueryHandler_H
#define GPOPT_CSubqueryHandler_H

#include "gpos/base.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CSubqueryHandler
//
//	@doc:
//		Helper class for transforming subquery expressions to Apply
//		expressions
//
//---------------------------------------------------------------------------
class CSubqueryHandler
{
public:
	// context in which subquery appears
	enum ESubqueryCtxt
	{
		EsqctxtValue,  // subquery appears in a project list
		EsqctxtFilter  // subquery appears in a comparison predicate
	};

private:
	//---------------------------------------------------------------------------
	//	@struct:
	//		SSubqueryDesc
	//
	//	@doc:
	//		Structure to maintain subquery descriptor
	//
	//---------------------------------------------------------------------------
	struct SSubqueryDesc
	{
		// subquery can return more than one row
		BOOL m_returns_set{false};

		// subquery has volatile functions
		BOOL m_fHasVolatileFunctions{false};

		// subquery has outer references
		BOOL m_fHasOuterRefs{false};

		// the returned column is an outer reference
		BOOL m_fReturnedPcrIsOuterRef{false};

		// subquery has skip level correlations -- when inner expression refers to columns defined above the immediate outer expression
		BOOL m_fHasSkipLevelCorrelations{false};

		// subquery has a single count(*)/count(Any) agg
		BOOL m_fHasCountAgg{false};

		// column defining count(*)/count(Any) agg, if any
		gpos::Ref<CColRef> m_pcrCountAgg{nullptr};

		//  does subquery project a count expression
		BOOL m_fProjectCount{false};

		// subquery is used in a value context
		BOOL m_fValueSubquery{false};

		// subquery requires correlated execution
		BOOL m_fCorrelatedExecution{false};

		// ctor
		SSubqueryDesc() = default;

		// set correlated execution flag
		void SetCorrelatedExecution();

	};	// struct SSubqueryDesc

	// memory pool
	CMemoryPool *m_mp;

	// enforce using correlated apply for unnesting subqueries
	BOOL m_fEnforceCorrelatedApply;

	// helper for adding nullness check, only if needed, to the given scalar expression
	static gpos::Ref<CExpression> PexprIsNotNull(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprLogical,
		gpos::Ref<CExpression> pexprScalar);

	// helper for adding a Project node with a const TRUE on top of the given expression
	static void AddProjectNode(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							   gpos::Ref<CExpression> *ppexprResult);

	// helper for creating a groupby node above or below the apply
	static gpos::Ref<CExpression> CreateGroupByNode(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprChild,
		CColRefArray *colref_array, BOOL fExistential,
		gpos::Ref<CColRef> colref, gpos::Ref<CExpression> pexprPredicate,
		gpos::Ref<CColRef> *pcrCount, gpos::Ref<CColRef> *pcrSum);

	// helper for creating an inner select expression when creating outer apply
	static gpos::Ref<CExpression> PexprInnerSelect(
		CMemoryPool *mp, const gpos::Ref<CColRef> pcrInner,
		gpos::Ref<CExpression> pexprInner,
		gpos::Ref<CExpression> pexprPredicate, BOOL *useNotNullableInnerOpt);

	// helper for creating outer apply expression for scalar subqueries
	static BOOL FCreateOuterApplyForScalarSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, gpos::Ref<CExpression> pexprSubquery,
		BOOL fOuterRefsUnderInner, gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);

	// helper for creating grouping columns for outer apply expression
	static BOOL FCreateGrpCols(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, BOOL fExistential,
		BOOL fOuterRefsUnderInner,
		CColRefArray **ppdrgpcr,  // output: constructed grouping columns
		BOOL *pfGbOnInner		  // output: is Gb created on inner expression
	);

	// helper for creating outer apply expression for existential/quantified subqueries
	static BOOL FCreateOuterApplyForExistOrQuant(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprInner, gpos::Ref<CExpression> pexprSubquery,
		gpos::Ref<CExpression> pexprPredicate, BOOL fOuterRefsUnderInner,
		gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar,
		BOOL useNotNullableInnerOpt);

	// helper for creating outer apply expression
	static BOOL FCreateOuterApply(CMemoryPool *mp,
								  gpos::Ref<CExpression> pexprOuter,
								  gpos::Ref<CExpression> pexprInner,
								  gpos::Ref<CExpression> pexprSubquery,
								  gpos::Ref<CExpression> pexprPredicate,
								  BOOL fOuterRefsUnderInner,
								  gpos::Ref<CExpression> *ppexprNewOuter,
								  gpos::Ref<CExpression> *ppexprResidualScalar,
								  BOOL useNotNullableInnerOpt);

	// helper for creating a scalar if expression used when generating an outer apply
	static gpos::Ref<CExpression> PexprScalarIf(
		CMemoryPool *mp, gpos::Ref<CColRef> pcrBool, gpos::Ref<CColRef> pcrSum,
		gpos::Ref<CColRef> pcrCount, gpos::Ref<CExpression> pexprSubquery,
		BOOL useNotNullableInnerOpt);

	// helper for creating a correlated apply expression for existential subquery
	static BOOL FCreateCorrelatedApplyForExistentialSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);

	// helper for creating a correlated apply expression for quantified subquery
	static BOOL FCreateCorrelatedApplyForQuantifiedSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);

	// helper for creating correlated apply expression
	static BOOL FCreateCorrelatedApplyForExistOrQuant(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);

	// create subquery descriptor
	static SSubqueryDesc *Psd(CMemoryPool *mp,
							  gpos::Ref<CExpression> pexprSubquery,
							  gpos::Ref<CExpression> pexprOuter,
							  const gpos::Ref<CColRef> pcrSubquery,
							  ESubqueryCtxt esqctxt);

	// detect subqueries with expressions over count aggregate similar to
	// (SELECT 'abc' || (SELECT count(*) from X))
	static BOOL FProjectCountSubquery(gpos::Ref<CExpression> pexprSubquery,
									  gpos::Ref<CColRef> ppcrCount);

	// given an input expression, replace all occurrences of given column with the given scalar expression
	static gpos::Ref<CExpression> PexprReplace(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRef> colref, gpos::Ref<CExpression> pexprSubquery);

	// remove a scalar subquery node from scalar tree
	BOOL FRemoveScalarSubquery(gpos::Ref<CExpression> pexprOuter,
							   gpos::Ref<CExpression> pexprSubquery,
							   ESubqueryCtxt esqctxt,
							   gpos::Ref<CExpression> *ppexprNewOuter,
							   gpos::Ref<CExpression> *ppexprResidualScalar);

	// helper to generate a correlated apply expression when needed
	static BOOL FGenerateCorrelatedApplyForScalarSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprSubquery, ESubqueryCtxt esqctxt,
		CSubqueryHandler::SSubqueryDesc *psd, BOOL fEnforceCorrelatedApply,
		gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);

	// internal function for removing a scalar subquery node from scalar tree
	static BOOL FRemoveScalarSubqueryInternal(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprSubquery, ESubqueryCtxt esqctxt,
		SSubqueryDesc *psd, BOOL fEnforceCorrelatedApply,
		gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);

	// remove a subquery ANY node from scalar tree
	BOOL FRemoveAnySubquery(gpos::Ref<CExpression> pexprOuter,
							gpos::Ref<CExpression> pexprSubquery,
							ESubqueryCtxt esqctxt,
							gpos::Ref<CExpression> *ppexprNewOuter,
							gpos::Ref<CExpression> *ppexprResidualScalar);

	// remove a subquery ALL node from scalar tree
	BOOL FRemoveAllSubquery(gpos::Ref<CExpression> pexprOuter,
							gpos::Ref<CExpression> pexprSubquery,
							ESubqueryCtxt esqctxt,
							gpos::Ref<CExpression> *ppexprNewOuter,
							gpos::Ref<CExpression> *ppexprResidualScalar);

	// add a limit 1 expression over given expression,
	// removing any existing limits
	static gpos::Ref<CExpression> AddOrReplaceLimitOne(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// remove a subquery EXISTS/NOT EXISTS node from scalar tree
	static BOOL FRemoveExistentialSubquery(
		CMemoryPool *mp, COperator::EOperatorId op_id,
		gpos::Ref<CExpression> pexprOuter, gpos::Ref<CExpression> pexprSubquery,
		ESubqueryCtxt esqctxt, gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);

	// remove a subquery EXISTS from scalar tree
	BOOL FRemoveExistsSubquery(gpos::Ref<CExpression> pexprOuter,
							   gpos::Ref<CExpression> pexprSubquery,
							   ESubqueryCtxt esqctxt,
							   gpos::Ref<CExpression> *ppexprNewOuter,
							   gpos::Ref<CExpression> *ppexprResidualScalar);

	// remove a subquery NOT EXISTS from scalar tree
	BOOL FRemoveNotExistsSubquery(gpos::Ref<CExpression> pexprOuter,
								  gpos::Ref<CExpression> pexprSubquery,
								  ESubqueryCtxt esqctxt,
								  gpos::Ref<CExpression> *ppexprNewOuter,
								  gpos::Ref<CExpression> *ppexprResidualScalar);

	// handle subqueries in scalar tree recursively
	BOOL FRecursiveHandler(gpos::Ref<CExpression> pexprOuter,
						   gpos::Ref<CExpression> pexprScalar,
						   ESubqueryCtxt esqctxt,
						   gpos::Ref<CExpression> *ppexprNewOuter,
						   gpos::Ref<CExpression> *ppexprNewScalar);

	// handle subqueries on a case-by-case basis
	BOOL FProcessScalarOperator(gpos::Ref<CExpression> pexprOuter,
								gpos::Ref<CExpression> pexprScalar,
								ESubqueryCtxt esqctxt,
								gpos::Ref<CExpression> *ppexprNewOuter,
								gpos::Ref<CExpression> *ppexprNewScalar);

#ifdef GPOS_DEBUG
	// assert valid values of arguments
	static void AssertValidArguments(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
		gpos::Ref<CExpression> pexprScalar,
		gpos::Ref<CExpression> *ppexprNewOuter,
		gpos::Ref<CExpression> *ppexprResidualScalar);
#endif	// GPOS_DEBUG

public:
	CSubqueryHandler(const CSubqueryHandler &) = delete;

	// ctor
	CSubqueryHandler(CMemoryPool *mp, BOOL fEnforceCorrelatedApply)
		: m_mp(mp), m_fEnforceCorrelatedApply(fEnforceCorrelatedApply)
	{
	}

	// build an expression for the quantified comparison of the subquery
	gpos::Ref<CExpression> PexprSubqueryPred(
		gpos::Ref<CExpression> pexprOuter, gpos::Ref<CExpression> pexprSubquery,
		gpos::Ref<CExpression> *ppexprResult,
		CSubqueryHandler::ESubqueryCtxt esqctxt);

	// main driver
	BOOL FProcess(
		gpos::Ref<CExpression> pexprOuter,	 // logical child of a SELECT node
		gpos::Ref<CExpression> pexprScalar,	 // scalar child of a SELECT node
		ESubqueryCtxt esqctxt,				 // context in which subquery occurs
		gpos::Ref<CExpression>
			*ppexprNewOuter,  // an Apply logical expression produced as output
		gpos::Ref<CExpression> *
			ppexprResidualScalar  // residual scalar expression produced as output
	);

};	// class CSubqueryHandler

}  // namespace gpopt

#endif	// !GPOPT_CSubqueryHandler_H

// EOF
