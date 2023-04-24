//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CXformSubqJoin2Apply.h
//
//	@doc:
//		Transform Inner Join to Apply
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformSubqJoin2Apply_H
#define GPOPT_CXformSubqJoin2Apply_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformSubqueryUnnest.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformSubqJoin2Apply
//
//	@doc:
//		Transform Inner Join with subq to Apply
//
//---------------------------------------------------------------------------
class CXformSubqJoin2Apply : public CXformSubqueryUnnest
{
private:
	// hash map between expression and a column reference
	using ExprToColRefMap =
		CHashMap<CExpression, CColRef, HashPtr<CExpression>,
				 EqualPtr<CExpression>, CleanupRelease<CExpression>,
				 CleanupNULL<CColRef>>;

	// helper to transform function
	void Transform(gpos::Ref<CXformContext> pxfctxt,
				   gpos::Ref<CXformResult> pxfres, gpos::Ref<CExpression> pexpr,
				   BOOL fEnforceCorrelatedApply) const override;

	// collect subqueries that exclusively use outer/inner child
	static void CollectSubqueries(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSetArray> pdrgpcrs,
		gpos::Ref<CExpressionArrays> pdrgpdrgpexprSubqs);

	// replace subqueries with scalar identifier based on given map
	static gpos::Ref<CExpression> PexprReplaceSubqueries(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar,
		gpos::Ref<ExprToColRefMap> phmexprcr);

	// push down subquery below join
	static gpos::Ref<CExpression> PexprSubqueryPushDown(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		BOOL fEnforceCorrelatedApply);

public:
	CXformSubqJoin2Apply(const CXformSubqJoin2Apply &) = delete;

	// ctor
	explicit CXformSubqJoin2Apply(CMemoryPool *mp);

	// ctor
	explicit CXformSubqJoin2Apply(gpos::Ref<CExpression> pexprPattern)
		: CXformSubqueryUnnest(pexprPattern)
	{
	}

	// dtor
	~CXformSubqJoin2Apply() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfSubqJoin2Apply;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformSubqJoin2Apply";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

};	// class CXformSubqJoin2Apply

}  // namespace gpopt

#endif	// !GPOPT_CXformSubqJoin2Apply_H

// EOF
