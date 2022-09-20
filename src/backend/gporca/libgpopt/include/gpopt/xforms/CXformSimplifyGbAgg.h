//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformSimplifyGbAgg.h
//
//	@doc:
//		Simplify an aggregate by splitting grouping columns into a set of
//		functional dependencies in preparation for pushing Gb below join
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformSimplifyGbAgg_H
#define GPOPT_CXformSimplifyGbAgg_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformSimplifyGbAgg
//
//	@doc:
//		Simplify an aggregate by splitting grouping columns into a set of
//		functional dependencies
//
//---------------------------------------------------------------------------
class CXformSimplifyGbAgg : public CXformExploration
{
private:
	// helper to check if GbAgg can be transformed to a Select
	static BOOL FDropGbAgg(CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
						   gpos::pointer<CXformResult *> pxfres);

public:
	CXformSimplifyGbAgg(const CXformSimplifyGbAgg &) = delete;

	// ctor
	explicit CXformSimplifyGbAgg(CMemoryPool *mp);

	// dtor
	~CXformSimplifyGbAgg() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfSimplifyGbAgg;
	}

	// return a string for xform name
	const CHAR *
	SzId() const override
	{
		return "CXformSimplifyGbAgg";
	}

	// Compatibility function for simplifying aggregates
	BOOL
	FCompatible(CXform::EXformId exfid) override
	{
		return (CXform::ExfSimplifyGbAgg != exfid) &&
			   (CXform::ExfSplitDQA != exfid) &&
			   (CXform::ExfSplitGbAgg != exfid) &&
			   (CXform::ExfEagerAgg != exfid);
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(gpos::pointer<CXformContext *>,
				   gpos::pointer<CXformResult *>,
				   gpos::pointer<CExpression *>) const override;

};	// class CXformSimplifyGbAgg

}  // namespace gpopt

#endif	// !GPOPT_CXformSimplifyGbAgg_H

// EOF
