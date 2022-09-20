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
	BOOL FInitChildCursors(CMemoryPool *mp,
						   gpos::pointer<CGroupExpression *> pgexpr,
						   CExpression *pexprPattern,
						   gpos::pointer<CExpressionArray *> pdrgpexpr);

	// advance cursors of child expressions
	BOOL FAdvanceChildCursors(CMemoryPool *mp,
							  gpos::pointer<CGroupExpression *> pgexpr,
							  CExpression *pexprPattern,
							  gpos::pointer<CExpression *> pexprLast,
							  gpos::pointer<CExpressionArray *> pdrgpexpr);

	// extraction of child expressions
	BOOL FExtractChildren(CMemoryPool *mp, CExpression *pexprPattern,
						  CGroupExpression *pgexprCursor,
						  CExpressionArray *pdrgpexpr);

	// move cursor
	static CGroupExpression *PgexprNext(
		gpos::pointer<CGroup *> pgroup,
		gpos::pointer<CGroupExpression *> pgexpr);

	// expand n-th child of pattern
	static CExpression *PexprExpandPattern(CExpression *pexpr, ULONG ulPos,
										   ULONG arity);

	// get binding for children
	BOOL FExtractChildren(CMemoryPool *mp,
						  gpos::pointer<CGroupExpression *> pgexpr,
						  CExpression *pexprPattern,
						  gpos::pointer<CExpression *> pexprLast,
						  gpos::pointer<CExpressionArray *> pdrgpexprChildren);

	// extract binding from a group
	gpos::owner<CExpression *> PexprExtract(
		CMemoryPool *mp, gpos::pointer<CGroup *> pgroup,
		CExpression *pexprPattern, gpos::pointer<CExpression *> pexprLast);

	// build expression
	static gpos::owner<CExpression *> PexprFinalize(
		CMemoryPool *mp, gpos::pointer<CGroupExpression *> pgexpr,
		gpos::owner<CExpressionArray *> pdrgpexprChildren);

	// private copy ctor
	CBinding(const CBinding &);

public:
	// ctor
	CBinding() = default;

	// dtor
	~CBinding() = default;

	// extract binding from group expression
	gpos::owner<CExpression *> PexprExtract(
		CMemoryPool *mp, gpos::pointer<CGroupExpression *> pgexpr,
		CExpression *pexprPatetrn, gpos::pointer<CExpression *> pexprLast);

};	// class CBinding

}  // namespace gpopt

#endif	// !GPOPT_CBinding_H

// EOF
