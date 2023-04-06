//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformSimplifySubquery.h
//
//	@doc:
//		Simplify existential/quantified subqueries by transforming
//		into count(*) subqueries
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformSimplifySubquery_H
#define GPOPT_CXformSimplifySubquery_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformSimplifySubquery
//
//	@doc:
//		Simplify existential/quantified subqueries by transforming
//		into count(*) subqueries
//
//---------------------------------------------------------------------------
class CXformSimplifySubquery : public CXformExploration
{
private:
	// definition of simplification function
	using FnSimplify = BOOL(CMemoryPool *, gpos::Ref<CExpression>,
							gpos::Ref<CExpression> *);

	// definition of matching function
	using FnMatch = BOOL(gpos::Ref<COperator>);

	// transform existential subqueries to count(*) subqueries
	static BOOL FSimplifyExistential(CMemoryPool *mp,
									 gpos::Ref<CExpression> pexprScalar,
									 gpos::Ref<CExpression> *ppexprNewScalar);

	// transform quantified subqueries to count(*) subqueries
	static BOOL FSimplifyQuantified(CMemoryPool *mp,
									gpos::Ref<CExpression> pexprScalar,
									gpos::Ref<CExpression> *ppexprNewScalar);

	// main driver, transform existential/quantified subqueries to count(*) subqueries
	static BOOL FSimplifySubqueryRecursive(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar,
		gpos::Ref<CExpression> *ppexprNewScalar, FnSimplify *pfnsimplify,
		FnMatch *pfnmatch);

	static gpos::Ref<CExpression> FSimplifySubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprInput,
		FnSimplify *pfnsimplify, FnMatch *pfnmatch);

public:
	CXformSimplifySubquery(const CXformSimplifySubquery &) = delete;

	// ctor
	explicit CXformSimplifySubquery(gpos::Ref<CExpression> pexprPattern);

	// dtor
	~CXformSimplifySubquery() override = default;

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(gpos::Ref<CXformContext> pxfctxt,
				   gpos::Ref<CXformResult> pxfres,
				   gpos::Ref<CExpression> pexpr) const override;


};	// class CXformSimplifySubquery

}  // namespace gpopt

#endif	// !GPOPT_CXformSimplifySubquery_H

// EOF
