//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CExpressionUtils.h
//
//	@doc:
//		Utility routines for transforming expressions
//
//	@owner:
//		,
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPOPT_CExpressionUtils_H
#define GPOPT_CExpressionUtils_H

#include "gpos/base.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

// fwd declarations
class CExpression;

//---------------------------------------------------------------------------
//	@class:
//		CExpressionUtils
//
//	@doc:
//		Utility routines for transforming expressions
//
//---------------------------------------------------------------------------
class CExpressionUtils
{
private:
	// unnest a given expression's child and append unnested nodes to given array
	static void UnnestChild(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							ULONG UlChildIndex, BOOL fAnd, BOOL fOr,
							BOOL fNotChildren,
							gpos::Ref<CExpressionArray> pdrgpexpr);

	// append the unnested children of given expression to given array
	static void AppendChildren(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							   gpos::Ref<CExpressionArray> pdrgpexpr);

	// return an array of expression children after being unnested
	static gpos::Ref<CExpressionArray> PdrgpexprUnnestChildren(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// push not expression one level down the given expression
	static gpos::Ref<CExpression> PexprPushNotOneLevel(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

public:
	// remove duplicate AND/OR children
	static gpos::Ref<CExpression> PexprDedupChildren(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// unnest AND/OR/NOT predicates
	static gpos::Ref<CExpression> PexprUnnest(CMemoryPool *mp,
											  gpos::Ref<CExpression> pexpr);

	// get constraints property from LogicalSelect operator with EXISTS/ANY subquery
	static gpos::Ref<CPropConstraint> GetPropConstraintFromSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);
};
}  // namespace gpopt

#endif	// !GPOPT_CExpressionUtils_H

// EOF
