//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 VMware, Inc. or its affiliates.
//
//	@filename:
//		CCastUtils.h
//
//	@doc:
//		Optimizer cast utility functions
//---------------------------------------------------------------------------
#ifndef GPOPT_CCastUtils_H
#define GPOPT_CCastUtils_H

#include "gpopt/base/CColRef.h"
#include "gpopt/operators/CExpression.h"

namespace gpmd
{
class IMDId;
}

namespace gpopt
{
using namespace gpos;

// general cast utility functions
class CCastUtils
{
public:
	// is the given expression a binary coercible cast of a scalar identifier for the given column
	static BOOL FBinaryCoercibleCastedScId(gpos::Ref<CExpression> pexpr,
										   gpos::Ref<CColRef> colref);

	// is the given expression a binary coercible cast of a scalar identifier
	static BOOL FBinaryCoercibleCastedScId(gpos::Ref<CExpression> pexpr);

	static BOOL FBinaryCoercibleCastedConst(gpos::Ref<CExpression> pexpr);

	// extract the column reference if the given expression a scalar identifier
	// or a cast of a scalar identifier or a function that casts a scalar identifier.
	// Else return NULL.
	static const gpos::Ref<CColRef> PcrExtractFromScIdOrCastScId(
		gpos::Ref<CExpression> pexpr);

	// cast the input column reference to the destination mdid
	static gpos::Ref<CExpression> PexprCast(CMemoryPool *mp,
											CMDAccessor *md_accessor,
											const gpos::Ref<CColRef> colref,
											gpos::Ref<IMDId> mdid_dest);

	// check whether the given expression is a binary coercible cast of something
	static BOOL FBinaryCoercibleCast(gpos::Ref<CExpression> pexpr);

	// check whether the given expression is a cast of something
	static BOOL FScalarCast(gpos::Ref<CExpression> pexpr);

	// return the given expression without any binary coercible casts
	// that exist on the top
	static gpos::Ref<CExpression> PexprWithoutBinaryCoercibleCasts(
		gpos::Ref<CExpression> pexpr);

	// add explicit casting to equality operations between compatible types
	static CExpressionArray *PdrgpexprCastEquality(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// helper to add explicit casting to left child of given equality predicate
	static gpos::Ref<CExpression> PexprAddCast(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprPred);

	// add explicit casting on the input expression to the destination type
	static gpos::Ref<CExpression> PexprCast(CMemoryPool *mp,
											CMDAccessor *md_accessor,
											gpos::Ref<CExpression> pexpr,
											gpos::Ref<IMDId> mdid_dest);
};	// class CCastUtils

}  // namespace gpopt

#endif	// !GPOPT_CCastUtils_H

// EOF
