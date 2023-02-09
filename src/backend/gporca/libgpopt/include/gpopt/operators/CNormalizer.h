//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2012 EMC Corp.
//
//	@filename:
//		CNormalizer.h
//
//	@doc:
//		Normalization of expression trees;
//		this currently includes predicates push down
//---------------------------------------------------------------------------
#ifndef GPOPT_CNormalizer_H
#define GPOPT_CNormalizer_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CNormalizer
//
//	@doc:
//		Normalization of expression trees
//
//---------------------------------------------------------------------------
class CNormalizer
{
private:
	//  return true if second expression is a child of first expression
	static BOOL FChild(gpos::Ref<CExpression> pexpr,
					   gpos::Ref<CExpression> pexprChild);

	// simplify outer joins
	static BOOL FSimplifySelectOnOuterJoin(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprOuterJoin,
		gpos::Ref<CExpression> pexprPred, gpos::Ref<CExpression> *ppexprResult);

	static BOOL FSimplifySelectOnFullJoin(CMemoryPool *mp,
										  gpos::Ref<CExpression> pexprFullJoin,
										  gpos::Ref<CExpression> pexprPred,
										  gpos::Ref<CExpression> *ppexprResult);

	// call normalizer recursively on expression children
	static gpos::Ref<CExpression> PexprRecursiveNormalize(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// check if a scalar predicate can be pushed through a logical expression
	static BOOL FPushable(gpos::Ref<CExpression> pexprLogical,
						  gpos::Ref<CExpression> pexprPred);

	// check if a scalar predicate can be pushed through the child of a sequence project expression
	static BOOL FPushableThruSeqPrjChild(gpos::Ref<CExpression> pexprSeqPrj,
										 gpos::Ref<CExpression> pexprPred);

	// check if a conjunct should be pushed through expression's outer child
	static BOOL FPushThruOuterChild(gpos::Ref<CExpression> pexprLogical);

	// return a Select expression, if needed, with a scalar condition made of given array of conjuncts
	static gpos::Ref<CExpression> PexprSelect(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpressionArray> pdrgpexpr);

	// push scalar expression through an expression with unary operator with scalar child
	static void PushThruUnaryWithScalarChild(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
		gpos::Ref<CExpression> pexprConj, gpos::Ref<CExpression> *ppexprResult);

	// push scalar expression through a sequence project expression
	static void PushThruSeqPrj(CMemoryPool *mp,
							   gpos::Ref<CExpression> pexprSeqPrj,
							   gpos::Ref<CExpression> pexprConj,
							   gpos::Ref<CExpression> *ppexprResult);

	// push scalar expression through a set operation
	static void PushThruSetOp(CMemoryPool *mp,
							  gpos::Ref<CExpression> pexprSetOp,
							  gpos::Ref<CExpression> pexprConj,
							  gpos::Ref<CExpression> *ppexprResult);

	// push a conjunct through a CTE anchor operator
	static void PushThruUnaryWithoutScalarChild(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
		gpos::Ref<CExpression> pexprConj, gpos::Ref<CExpression> *ppexprResult);

	// push scalar expression through a select expression
	static void PushThruSelect(CMemoryPool *mp,
							   gpos::Ref<CExpression> pexprSelect,
							   gpos::Ref<CExpression> pexprConj,
							   gpos::Ref<CExpression> *ppexprResult);

	// split the given conjunct into pushable and unpushable predicates
	static void SplitConjunct(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprConj,
		gpos::Ref<CExpressionArray> *ppdrgpexprPushable,
		gpos::Ref<CExpressionArray> *ppdrgpexprUnpushable);

	// split the given conjunct into pushable and unpushable predicates for a sequence project expression
	static void SplitConjunctForSeqPrj(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprSeqPrj,
		gpos::Ref<CExpression> pexprConj,
		gpos::Ref<CExpressionArray> *ppdrgpexprPushable,
		gpos::Ref<CExpressionArray> *ppdrgpexprUnpushable);

	// push scalar expression through left outer join children
	static void PushThruOuterChild(CMemoryPool *mp,
								   gpos::Ref<CExpression> pexpr,
								   gpos::Ref<CExpression> pexprConj,
								   gpos::Ref<CExpression> *ppexprResult);

	// push scalar expression through a join expression
	static void PushThruJoin(CMemoryPool *mp, gpos::Ref<CExpression> pexprJoin,
							 gpos::Ref<CExpression> pexprConj,
							 gpos::Ref<CExpression> *ppexprResult);

	// push scalar expression through logical expression
	static void PushThru(CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
						 gpos::Ref<CExpression> pexprConj,
						 gpos::Ref<CExpression> *ppexprResult);

	// push an array of conjuncts through logical expression, and compute an array of remaining conjuncts
	static void PushThru(CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
						 CExpressionArray *pdrgpexprConjuncts,
						 gpos::Ref<CExpression> *ppexprResult,
						 gpos::Ref<CExpressionArray> *ppdrgpexprRemaining);

	// private copy ctor
	CNormalizer(const CNormalizer &);

	// pull logical projects as far up the logical tree as possible, and
	// combine consecutive projects if possible
	static gpos::Ref<CExpression> PexprPullUpAndCombineProjects(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, BOOL *pfSuccess);

	// pull up project elements from the given projection expression that do not
	// exist in the given used columns set
	static gpos::Ref<CExpression> PexprPullUpProjectElements(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRefSet *pcrsUsed,
		CColRefSet *pcrsOutput, CExpressionArray *pdrgpexprPrElPullUp);

#ifdef GPOS_DEBUG
	// check if the columns used by the operator are a subset of its input columns
	static BOOL FLocalColsSubsetOfInputCols(CMemoryPool *mp,
											gpos::Ref<CExpression> pexpr);
#endif	//GPOS_DEBUG

public:
	// main driver
	static gpos::Ref<CExpression> PexprNormalize(CMemoryPool *mp,
												 gpos::Ref<CExpression> pexpr);

	// normalize joins so that they are on colrefs
	static gpos::Ref<CExpression> PexprNormalizeJoins(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// pull logical projects as far up the logical tree as possible, and
	// combine consecutive projects if possible
	static gpos::Ref<CExpression> PexprPullUpProjections(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

};	// class CNormalizer
}  // namespace gpopt


#endif	// !GPOPT_CNormalizer_H

// EOF
