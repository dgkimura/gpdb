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
#include "gpos/common/owner.h"

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
		CColRef *m_pcrCountAgg{nullptr};

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
	static gpos::owner<CExpression *> PexprIsNotNull(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprLogical, CExpression *pexprScalar);

	// helper for adding a Project node with a const TRUE on top of the given expression
	static void AddProjectNode(CMemoryPool *mp,
							   gpos::owner<CExpression *> pexpr,
							   gpos::owner<CExpression *> *ppexprResult);

	// helper for creating a groupby node above or below the apply
	static gpos::owner<CExpression *> CreateGroupByNode(
		CMemoryPool *mp, CExpression *pexprChild,
		gpos::owner<CColRefArray *> colref_array, BOOL fExistential,
		CColRef *colref, CExpression *pexprPredicate, CColRef **pcrCount,
		CColRef **pcrSum);

	// helper for creating an inner select expression when creating outer apply
	static gpos::owner<CExpression *> PexprInnerSelect(
		CMemoryPool *mp, const CColRef *pcrInner, CExpression *pexprInner,
		CExpression *pexprPredicate, BOOL *useNotNullableInnerOpt);

	// helper for creating outer apply expression for scalar subqueries
	static BOOL FCreateOuterApplyForScalarSubquery(
		CMemoryPool *mp, CExpression *pexprOuter, CExpression *pexprInner,
		gpos::pointer<CExpression *> pexprSubquery, BOOL fOuterRefsUnderInner,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// helper for creating grouping columns for outer apply expression
	static BOOL FCreateGrpCols(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprInner, BOOL fExistential,
		BOOL fOuterRefsUnderInner,
		gpos::owner<CColRefArray *>
			*ppdrgpcr,	   // output: constructed grouping columns
		BOOL *pfGbOnInner  // output: is Gb created on inner expression
	);

	// helper for creating outer apply expression for existential/quantified subqueries
	static BOOL FCreateOuterApplyForExistOrQuant(
		CMemoryPool *mp, CExpression *pexprOuter, CExpression *pexprInner,
		gpos::pointer<CExpression *> pexprSubquery, CExpression *pexprPredicate,
		BOOL fOuterRefsUnderInner, gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar,
		BOOL useNotNullableInnerOpt);

	// helper for creating outer apply expression
	static BOOL FCreateOuterApply(
		CMemoryPool *mp, CExpression *pexprOuter, CExpression *pexprInner,
		gpos::pointer<CExpression *> pexprSubquery, CExpression *pexprPredicate,
		BOOL fOuterRefsUnderInner, gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar,
		BOOL useNotNullableInnerOpt);

	// helper for creating a scalar if expression used when generating an outer apply
	static gpos::owner<CExpression *> PexprScalarIf(
		CMemoryPool *mp, CColRef *pcrBool, CColRef *pcrSum, CColRef *pcrCount,
		gpos::pointer<CExpression *> pexprSubquery,
		BOOL useNotNullableInnerOpt);

	// helper for creating a correlated apply expression for existential subquery
	static BOOL FCreateCorrelatedApplyForExistentialSubquery(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// helper for creating a correlated apply expression for quantified subquery
	static BOOL FCreateCorrelatedApplyForQuantifiedSubquery(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// helper for creating correlated apply expression
	static BOOL FCreateCorrelatedApplyForExistOrQuant(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// create subquery descriptor
	static SSubqueryDesc *Psd(CMemoryPool *mp,
							  gpos::pointer<CExpression *> pexprSubquery,
							  gpos::pointer<CExpression *> pexprOuter,
							  const CColRef *pcrSubquery,
							  ESubqueryCtxt esqctxt);

	// detect subqueries with expressions over count aggregate similar to
	// (SELECT 'abc' || (SELECT count(*) from X))
	static BOOL FProjectCountSubquery(
		gpos::pointer<CExpression *> pexprSubquery, CColRef *ppcrCount);

	// given an input expression, replace all occurrences of given column with the given scalar expression
	static gpos::owner<CExpression *> PexprReplace(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		gpos::pointer<CExpression *> pexprSubquery);

	// remove a scalar subquery node from scalar tree
	BOOL FRemoveScalarSubquery(
		CExpression *pexprOuter, gpos::pointer<CExpression *> pexprSubquery,
		ESubqueryCtxt esqctxt, gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// helper to generate a correlated apply expression when needed
	static BOOL FGenerateCorrelatedApplyForScalarSubquery(
		CMemoryPool *mp, CExpression *pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		CSubqueryHandler::SSubqueryDesc *psd, BOOL fEnforceCorrelatedApply,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// internal function for removing a scalar subquery node from scalar tree
	static BOOL FRemoveScalarSubqueryInternal(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		SSubqueryDesc *psd, BOOL fEnforceCorrelatedApply,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// remove a subquery ANY node from scalar tree
	BOOL FRemoveAnySubquery(gpos::owner<CExpression *> pexprOuter,
							gpos::pointer<CExpression *> pexprSubquery,
							ESubqueryCtxt esqctxt,
							gpos::owner<CExpression *> *ppexprNewOuter,
							gpos::owner<CExpression *> *ppexprResidualScalar);

	// remove a subquery ALL node from scalar tree
	BOOL FRemoveAllSubquery(gpos::owner<CExpression *> pexprOuter,
							gpos::pointer<CExpression *> pexprSubquery,
							ESubqueryCtxt esqctxt,
							gpos::owner<CExpression *> *ppexprNewOuter,
							gpos::owner<CExpression *> *ppexprResidualScalar);

	// add a limit 1 expression over given expression,
	// removing any existing limits
	static gpos::owner<CExpression *> AddOrReplaceLimitOne(CMemoryPool *mp,
														   CExpression *pexpr);

	// remove a subquery EXISTS/NOT EXISTS node from scalar tree
	static BOOL FRemoveExistentialSubquery(
		CMemoryPool *mp, COperator::EOperatorId op_id, CExpression *pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// remove a subquery EXISTS from scalar tree
	BOOL FRemoveExistsSubquery(
		gpos::owner<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// remove a subquery NOT EXISTS from scalar tree
	BOOL FRemoveNotExistsSubquery(
		gpos::owner<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprSubquery, ESubqueryCtxt esqctxt,
		gpos::owner<CExpression *> *ppexprNewOuter,
		gpos::owner<CExpression *> *ppexprResidualScalar);

	// handle subqueries in scalar tree recursively
	BOOL FRecursiveHandler(CExpression *pexprOuter,
						   gpos::pointer<CExpression *> pexprScalar,
						   ESubqueryCtxt esqctxt,
						   gpos::owner<CExpression *> *ppexprNewOuter,
						   gpos::owner<CExpression *> *ppexprNewScalar);

	// handle subqueries on a case-by-case basis
	BOOL FProcessScalarOperator(CExpression *pexprOuter,
								gpos::pointer<CExpression *> pexprScalar,
								ESubqueryCtxt esqctxt,
								gpos::owner<CExpression *> *ppexprNewOuter,
								gpos::owner<CExpression *> *ppexprNewScalar);

#ifdef GPOS_DEBUG
	// assert valid values of arguments
	static void AssertValidArguments(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
		gpos::pointer<CExpression *> pexprScalar,
		gpos::pointer<CExpression *> *ppexprNewOuter,
		gpos::pointer<CExpression *> *ppexprResidualScalar);
#endif	// GPOS_DEBUG

public:
	CSubqueryHandler(const CSubqueryHandler &) = delete;

	// ctor
	CSubqueryHandler(CMemoryPool *mp, BOOL fEnforceCorrelatedApply)
		: m_mp(mp), m_fEnforceCorrelatedApply(fEnforceCorrelatedApply)
	{
	}

	// build an expression for the quantified comparison of the subquery
	gpos::owner<CExpression *> PexprSubqueryPred(
		CExpression *pexprOuter, gpos::pointer<CExpression *> pexprSubquery,
		gpos::owner<CExpression *> *ppexprResult,
		CSubqueryHandler::ESubqueryCtxt esqctxt);

	// main driver
	BOOL FProcess(
		CExpression *pexprOuter,  // logical child of a SELECT node
		gpos::pointer<CExpression *>
			pexprScalar,		// scalar child of a SELECT node
		ESubqueryCtxt esqctxt,	// context in which subquery occurs
		gpos::owner<CExpression *>
			*ppexprNewOuter,  // an Apply logical expression produced as output
		gpos::owner<CExpression *> *
			ppexprResidualScalar  // residual scalar expression produced as output
	);

};	// class CSubqueryHandler

}  // namespace gpopt

#endif	// !GPOPT_CSubqueryHandler_H

// EOF
