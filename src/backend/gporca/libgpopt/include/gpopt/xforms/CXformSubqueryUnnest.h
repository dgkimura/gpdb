//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformSubqueryUnnest.h
//
//	@doc:
//		Base class for subquery unnesting xforms
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformSubqueryUnnest_H
#define GPOPT_CXformSubqueryUnnest_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformSubqueryUnnest
//
//	@doc:
//		Base class for subquery unnesting xforms
//
//---------------------------------------------------------------------------
class CXformSubqueryUnnest : public CXformExploration
{
private:
protected:
	// helper for subquery unnesting
	static gpos::owner<CExpression *> PexprSubqueryUnnest(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		BOOL fEnforceCorrelatedApply);

	// actual transform
	virtual void Transform(gpos::pointer<CXformContext *> pxfctxt,
						   gpos::pointer<CXformResult *> pxfres,
						   gpos::pointer<CExpression *> pexpr,
						   BOOL fEnforceCorrelatedApply) const;

public:
	CXformSubqueryUnnest(const CXformSubqueryUnnest &) = delete;

	// ctor
	explicit CXformSubqueryUnnest(gpos::owner<CExpression *> pexprPattern)
		: CXformExploration(std::move(pexprPattern))
	{
	}

	// dtor
	~CXformSubqueryUnnest() override = default;

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(gpos::pointer<CXformContext *> pxfctxt,
				   gpos::pointer<CXformResult *> pxfres,
				   gpos::pointer<CExpression *> pexpr) const override;

	// is transformation a subquery unnesting (Subquery To Apply) xform?
	BOOL
	FSubqueryUnnesting() const override
	{
		return true;
	}

};	// class CXformSubqueryUnnest

}  // namespace gpopt

#endif	// !GPOPT_CXformSubqueryUnnest_H

// EOF
