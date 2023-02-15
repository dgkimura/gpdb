//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CBinding.h
//
//	@doc:
//		Binding mechanism to extract expression from Memo according to pattern
//---------------------------------------------------------------------------
#ifndef GPOPT_CBinding_H
#define GPOPT_CBinding_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

// fwd declaration
class CGroupExpression;
class CGroup;

//---------------------------------------------------------------------------
//	@class:
//		CBinding
//
//	@doc:
//		Binding class used to iteratively generate expressions from the
//		memo so that they match a given pattern
//
//
//---------------------------------------------------------------------------
class CBinding
{
private:
	// initialize cursors of child expressions
	BOOL FInitChildCursors(CMemoryPool *mp, gpos::Ref<CGroupExpression> pgexpr,
						   gpos::Ref<CExpression> pexprPattern,
						   gpos::Ref<CExpressionArray> pdrgpexpr);

	// advance cursors of child expressions
	BOOL FAdvanceChildCursors(CMemoryPool *mp,
							  gpos::Ref<CGroupExpression> pgexpr,
							  gpos::Ref<CExpression> pexprPattern,
							  gpos::Ref<CExpression> pexprLast,
							  gpos::Ref<CExpressionArray> pdrgpexpr);

	// extraction of child expressions
	BOOL FExtractChildren(CMemoryPool *mp, gpos::Ref<CExpression> pexprPattern,
						  gpos::Ref<CGroupExpression> pgexprCursor,
						  gpos::Ref<CExpressionArray> pdrgpexpr);

	// move cursor
	static gpos::Ref<CGroupExpression> PgexprNext(
		gpos::Ref<CGroup> pgroup, gpos::Ref<CGroupExpression> pgexpr);

	// expand n-th child of pattern
	static gpos::Ref<CExpression> PexprExpandPattern(
		gpos::Ref<CExpression> pexpr, ULONG ulPos, ULONG arity);

	// get binding for children
	BOOL FExtractChildren(CMemoryPool *mp, gpos::Ref<CGroupExpression> pgexpr,
						  gpos::Ref<CExpression> pexprPattern,
						  gpos::Ref<CExpression> pexprLast,
						  gpos::Ref<CExpressionArray> pdrgpexprChildren);

	// extract binding from a group
	gpos::Ref<CExpression> PexprExtract(CMemoryPool *mp,
										gpos::Ref<CGroup> pgroup,
										gpos::Ref<CExpression> pexprPattern,
										gpos::Ref<CExpression> pexprLast);

	// build expression
	static gpos::Ref<CExpression> PexprFinalize(
		CMemoryPool *mp, gpos::Ref<CGroupExpression> pgexpr,
		gpos::Ref<CExpressionArray> pdrgpexprChildren);

	// private copy ctor
	CBinding(const CBinding &);

public:
	// ctor
	CBinding() = default;

	// dtor
	~CBinding() = default;

	// extract binding from group expression
	gpos::Ref<CExpression> PexprExtract(CMemoryPool *mp,
										gpos::Ref<CGroupExpression> pgexpr,
										gpos::Ref<CExpression> pexprPatetrn,
										gpos::Ref<CExpression> pexprLast);

};	// class CBinding

}  // namespace gpopt

#endif	// !GPOPT_CBinding_H

// EOF
