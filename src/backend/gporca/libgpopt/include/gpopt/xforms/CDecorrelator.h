//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDecorrelator.h
//
//	@doc:
//		Decorrelation processor
//---------------------------------------------------------------------------
#ifndef GPOPT_CDecorrelator_H
#define GPOPT_CDecorrelator_H

#include "gpos/base.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDecorrelator
//
//	@doc:
//		Helper class for extracting correlated expressions
//
//---------------------------------------------------------------------------
class CDecorrelator
{
private:
	// definition of operator processor
	using FnProcessor = BOOL(CMemoryPool *, gpos::Ref<CExpression>, BOOL,
							 gpos::Ref<CExpression> *,
							 gpos::Ref<CExpressionArray>,
							 gpos::Ref<CColRefSet>);

	//---------------------------------------------------------------------------
	//	@struct:
	//		SOperatorProcessor
	//
	//	@doc:
	//		Mapping of operator to a processor function
	//
	//---------------------------------------------------------------------------
	struct SOperatorProcessor
	{
		// scalar operator id
		COperator::EOperatorId m_eopid;

		// pointer to handler function
		FnProcessor *m_pfnp;

	};	// struct SOperatorHandler

	// helper to check if correlations below join are valid to be pulled-up
	static BOOL FPullableCorrelations(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpressionArray> pdrgpexpr,
		gpos::Ref<CExpressionArray> pdrgpexprCorrelations);

	// check if scalar operator can be delayed
	static BOOL FDelayableScalarOp(gpos::Ref<CExpression> pexprScalar);

	// check if scalar expression can be lifted
	static BOOL FDelayable(gpos::Ref<CExpression> pexprLogical,
						   gpos::Ref<CExpression> pexprScalar,
						   BOOL fEqualityOnly);

	// switch function for all operators
	static BOOL FProcessOperator(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL fEqualityOnly,
		gpos::Ref<CExpression> *ppexprDecorrelated,
		gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
		gpos::Ref<CColRefSet> outerRefsToRemove);

	// processor for predicates
	static BOOL FProcessPredicate(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
		gpos::Ref<CExpression> pexprScalar, BOOL fEqualityOnly,
		gpos::Ref<CExpression> *ppexprDecorrelated,
		gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
		gpos::Ref<CColRefSet> outerRefsToRemove);

	// processor for select operators
	static BOOL FProcessSelect(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL fEqualityOnly,
		gpos::Ref<CExpression> *ppexprDecorrelated,
		gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
		gpos::Ref<CColRefSet> outerRefsToRemove);


	// processor for aggregates
	static BOOL FProcessGbAgg(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							  BOOL fEqualityOnly,
							  gpos::Ref<CExpression> *ppexprDecorrelated,
							  gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
							  gpos::Ref<CColRefSet> outerRefsToRemove);

	// processor for joins (inner/n-ary)
	static BOOL FProcessJoin(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							 BOOL fEqualityOnly,
							 gpos::Ref<CExpression> *ppexprDecorrelated,
							 gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
							 gpos::Ref<CColRefSet> outerRefsToRemove);


	// processor for projects
	static BOOL FProcessProject(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL fEqualityOnly,
		gpos::Ref<CExpression> *ppexprDecorrelated,
		gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
		gpos::Ref<CColRefSet> outerRefsToRemove);

	// processor for assert
	static BOOL FProcessAssert(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL fEqualityOnly,
		gpos::Ref<CExpression> *ppexprDecorrelated,
		gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
		gpos::Ref<CColRefSet> outerRefsToRemove);

	// processor for MaxOneRow
	static BOOL FProcessMaxOneRow(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL fEqualityOnly,
		gpos::Ref<CExpression> *ppexprDecorrelated,
		gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
		gpos::Ref<CColRefSet> outerRefsToRemove);

	// processor for limits
	static BOOL FProcessLimit(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							  BOOL fEqualityOnly,
							  gpos::Ref<CExpression> *ppexprDecorrelated,
							  gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
							  gpos::Ref<CColRefSet> outerRefsToRemove);

public:
	// private dtor
	virtual ~CDecorrelator() = delete;

	// private ctor
	CDecorrelator() = delete;

	CDecorrelator(const CDecorrelator &) = delete;

	// main handler
	static BOOL FProcess(CMemoryPool *mp, gpos::Ref<CExpression> pexprOrig,
						 BOOL fEqualityOnly,
						 gpos::Ref<CExpression> *ppexprDecorrelated,
						 gpos::Ref<CExpressionArray> pdrgpexprCorrelations,
						 gpos::Ref<CColRefSet> outerRefsToRemove);

};	// class CDecorrelator

}  // namespace gpopt

#endif	// !GPOPT_CDecorrelator_H

// EOF
