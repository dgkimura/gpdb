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
	static BOOL FChild(gpos::pointer<CExpression *> pexpr,
					   gpos::pointer<CExpression *> pexprChild);

	// simplify outer joins
	static BOOL FSimplifySelectOnOuterJoin(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuterJoin,
		gpos::pointer<CExpression *> pexprPred,
		gpos::owner<CExpression *> *ppexprResult);

	static BOOL FSimplifySelectOnFullJoin(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprFullJoin,
		CExpression *pexprPred, gpos::owner<CExpression *> *ppexprResult);

	// call normalizer recursively on expression children
	static gpos::owner<CExpression *> PexprRecursiveNormalize(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// check if a scalar predicate can be pushed through a logical expression
	static BOOL FPushable(gpos::pointer<CExpression *> pexprLogical,
						  gpos::pointer<CExpression *> pexprPred);

	// check if a scalar predicate can be pushed through the child of a sequence project expression
	static BOOL FPushableThruSeqPrjChild(
		gpos::pointer<CExpression *> pexprSeqPrj,
		gpos::pointer<CExpression *> pexprPred);

	// check if a conjunct should be pushed through expression's outer child
	static BOOL FPushThruOuterChild(gpos::pointer<CExpression *> pexprLogical);

	// return a Select expression, if needed, with a scalar condition made of given array of conjuncts
	static gpos::owner<CExpression *> PexprSelect(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr,
		gpos::owner<CExpressionArray *> pdrgpexpr);

	// push scalar expression through an expression with unary operator with scalar child
	static void PushThruUnaryWithScalarChild(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprLogical,
		gpos::pointer<CExpression *> pexprConj,
		gpos::owner<CExpression *> *ppexprResult);

	// push scalar expression through a sequence project expression
	static void PushThruSeqPrj(CMemoryPool *mp,
							   gpos::pointer<CExpression *> pexprSeqPrj,
							   gpos::pointer<CExpression *> pexprConj,
							   gpos::owner<CExpression *> *ppexprResult);

	// push scalar expression through a set operation
	static void PushThruSetOp(CMemoryPool *mp,
							  gpos::pointer<CExpression *> pexprSetOp,
							  CExpression *pexprConj,
							  gpos::owner<CExpression *> *ppexprResult);

	// push a conjunct through a CTE anchor operator
	static void PushThruUnaryWithoutScalarChild(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprLogical,
		gpos::pointer<CExpression *> pexprConj,
		gpos::owner<CExpression *> *ppexprResult);

	// push scalar expression through a select expression
	static void PushThruSelect(CMemoryPool *mp,
							   gpos::pointer<CExpression *> pexprSelect,
							   gpos::pointer<CExpression *> pexprConj,
							   gpos::owner<CExpression *> *ppexprResult);

	// split the given conjunct into pushable and unpushable predicates
	static void SplitConjunct(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CExpression *> pexprConj,
		gpos::owner<CExpressionArray *> *ppdrgpexprPushable,
		gpos::owner<CExpressionArray *> *ppdrgpexprUnpushable);

	// split the given conjunct into pushable and unpushable predicates for a sequence project expression
	static void SplitConjunctForSeqPrj(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprSeqPrj,
		gpos::pointer<CExpression *> pexprConj,
		gpos::owner<CExpressionArray *> *ppdrgpexprPushable,
		gpos::owner<CExpressionArray *> *ppdrgpexprUnpushable);

	// push scalar expression through left outer join children
	static void PushThruOuterChild(CMemoryPool *mp, CExpression *pexpr,
								   CExpression *pexprConj,
								   gpos::owner<CExpression *> *ppexprResult);

	// push scalar expression through a join expression
	static void PushThruJoin(CMemoryPool *mp, CExpression *pexprJoin,
							 CExpression *pexprConj,
							 gpos::owner<CExpression *> *ppexprResult);

	// push scalar expression through logical expression
	static void PushThru(CMemoryPool *mp, CExpression *pexprLogical,
						 CExpression *pexprConj,
						 gpos::owner<CExpression *> *ppexprResult);

	// push an array of conjuncts through logical expression, and compute an array of remaining conjuncts
	static void PushThru(CMemoryPool *mp, CExpression *pexprLogical,
						 gpos::pointer<CExpressionArray *> pdrgpexprConjuncts,
						 gpos::owner<CExpression *> *ppexprResult,
						 gpos::owner<CExpressionArray *> *ppdrgpexprRemaining);

	// private copy ctor
	CNormalizer(const CNormalizer &);

	// pull logical projects as far up the logical tree as possible, and
	// combine consecutive projects if possible
	static gpos::owner<CExpression *> PexprPullUpAndCombineProjects(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, BOOL *pfSuccess);

	// pull up project elements from the given projection expression that do not
	// exist in the given used columns set
	static gpos::owner<CExpression *> PexprPullUpProjectElements(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefSet *> pcrsUsed,
		gpos::pointer<CColRefSet *> pcrsOutput,
		gpos::pointer<CExpressionArray *> pdrgpexprPrElPullUp);

#ifdef GPOS_DEBUG
	// check if the columns used by the operator are a subset of its input columns
	static BOOL FLocalColsSubsetOfInputCols(CMemoryPool *mp,
											gpos::pointer<CExpression *> pexpr);
#endif	//GPOS_DEBUG

public:
	// main driver
	static gpos::owner<CExpression *> PexprNormalize(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// normalize joins so that they are on colrefs
	static CExpression *PexprNormalizeJoins(CMemoryPool *mp,
											CExpression *pexpr);

	// pull logical projects as far up the logical tree as possible, and
	// combine consecutive projects if possible
	static gpos::owner<CExpression *> PexprPullUpProjections(
		CMemoryPool *mp, CExpression *pexpr);

};	// class CNormalizer
}  // namespace gpopt


#endif	// !GPOPT_CNormalizer_H

// EOF
