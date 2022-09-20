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
#include "gpos/common/owner.h"

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
	using FnSimplify = BOOL(CMemoryPool *, gpos::pointer<CExpression *>,
							gpos::owner<CExpression *> *);

	// definition of matching function
	using FnMatch = BOOL(gpos::pointer<COperator *>);

	// transform existential subqueries to count(*) subqueries
	static BOOL FSimplifyExistential(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprScalar,
		gpos::owner<CExpression *> *ppexprNewScalar);

	// transform quantified subqueries to count(*) subqueries
	static BOOL FSimplifyQuantified(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprScalar,
		gpos::owner<CExpression *> *ppexprNewScalar);

	// main driver, transform existential/quantified subqueries to count(*) subqueries
	static BOOL FSimplifySubqueryRecursive(
		CMemoryPool *mp, CExpression *pexprScalar,
		gpos::owner<CExpression *> *ppexprNewScalar, FnSimplify *pfnsimplify,
		FnMatch *pfnmatch);

	static gpos::owner<CExpression *> FSimplifySubquery(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprInput,
		FnSimplify *pfnsimplify, FnMatch *pfnmatch);

public:
	CXformSimplifySubquery(const CXformSimplifySubquery &) = delete;

	// ctor
	explicit CXformSimplifySubquery(gpos::owner<CExpression *> pexprPattern);

	// dtor
	~CXformSimplifySubquery() override = default;

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(gpos::pointer<CXformContext *> pxfctxt,
				   gpos::pointer<CXformResult *> pxfres,
				   gpos::pointer<CExpression *> pexpr) const override;


};	// class CXformSimplifySubquery

}  // namespace gpopt

#endif	// !GPOPT_CXformSimplifySubquery_H

// EOF
