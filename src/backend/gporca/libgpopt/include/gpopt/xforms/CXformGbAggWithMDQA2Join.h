//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformGbAggWithMDQA2Join.h
//
//	@doc:
//		Transform a GbAgg with multiple distinct qualified aggregates (MDQAs)
//		to a join tree with single DQA leaves
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformGbAggWithMDQA2Join_H
#define GPOPT_CXformGbAggWithMDQA2Join_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformGbAggWithMDQA2Join
//
//	@doc:
//		Transform a GbAgg with multiple distinct qualified aggregates (MDQAs)
//		to a join tree with single DQA leaves
//
//---------------------------------------------------------------------------
class CXformGbAggWithMDQA2Join : public CXformExploration
{
private:
	static gpos::owner<CExpression *> PexprMDQAs2Join(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// expand GbAgg with multiple distinct aggregates into a join of single distinct
	// aggregates
	static gpos::owner<CExpression *> PexprExpandMDQAs(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// main transformation function driver
	static gpos::owner<CExpression *> PexprTransform(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

public:
	CXformGbAggWithMDQA2Join(const CXformGbAggWithMDQA2Join &) = delete;

	// ctor
	explicit CXformGbAggWithMDQA2Join(CMemoryPool *mp);

	// dtor
	~CXformGbAggWithMDQA2Join() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfGbAggWithMDQA2Join;
	}

	// return a string for xform name
	const CHAR *
	SzId() const override
	{
		return "CXformGbAggWithMDQA2Join";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(gpos::pointer<CXformContext *>,
				   gpos::pointer<CXformResult *>,
				   gpos::pointer<CExpression *>) const override;

	// return true if xform should be applied only once
	BOOL IsApplyOnce() override;

};	// class CXformGbAggWithMDQA2Join

}  // namespace gpopt

#endif	// !GPOPT_CXformGbAggWithMDQA2Join_H

// EOF
