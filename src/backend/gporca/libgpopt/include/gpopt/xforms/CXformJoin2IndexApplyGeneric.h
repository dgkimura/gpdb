//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2020 VMware and affiliates, Inc.
//
// Transform a join into an index apply. Allow a variety of nodes on
// the innser side, including a mandatory get, plus optional select,
// project and aggregate nodes.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformJoin2IndexApplyGeneric_H
#define GPOPT_CXformJoin2IndexApplyGeneric_H

#include "gpos/base.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/xforms/CXformJoin2IndexApply.h"

namespace gpopt
{
	using namespace gpos;

	class CXformJoin2IndexApplyGeneric : public CXformJoin2IndexApply
	{
	private:

		// no copy ctor
		CXformJoin2IndexApplyGeneric(const CXformJoin2IndexApplyGeneric &) = delete;

		// Can we transform left outer join to left outer index apply?
		BOOL
		FCanLeftOuterIndexApply
		(
		 CMemoryPool *mp,
		 CExpression *pexprInner,
		 CExpression *pexprScalar,
		 CTableDescriptor *ptabDesc,
		 const CColRefSet *pcrsDist
		 ) const;

	public:

		// ctor
		explicit
		CXformJoin2IndexApplyGeneric(CMemoryPool *mp)
		:
		// pattern
		CXformJoin2IndexApply
		(
		 GPOS_NEW(mp) CExpression
		 (
		  mp,
		  GPOS_NEW(mp) CPatternNode(mp, CPatternNode::EmtMatchInnerOrLeftOuterJoin),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)), // outer child
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)), // inner child
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))  // predicate tree operator,
		 )
		)
		{}

		// dtor
		virtual
		~CXformJoin2IndexApplyGeneric()
		{}

		// ident accessors
		virtual
		EXformId Exfid() const
		{
			return ExfJoin2IndexApplyGeneric;
		}

		virtual
		const CHAR *SzId() const
		{
			return "CXformJoin2IndexApplyGeneric";
		}

		virtual
		EXformPromise Exfp(CExpressionHandle &exprhdl) const;

		// actual transform
		virtual
		void Transform(CXformContext *pxfctxt, CXformResult *pxfres, CExpression *pexpr) const;

		// Return true if xform should be applied only once.
		// For now return true. We may need to revisit this if we find that
		// there are multiple bindings and we miss interesting bindings because
		// we extract only one of them.
		virtual
		BOOL IsApplyOnce()
		{
			return true;
		}

		// TODO: Remove these later??
		virtual
		CLogicalJoin *PopLogicalJoin(CMemoryPool *) const { return NULL; }

		virtual
		CLogicalApply *PopLogicalApply(CMemoryPool *, CColRefArray *) const { return NULL; }

	}; // class CXformJoin2IndexApplyBase

}

#endif // !GPOPT_CXformJoin2IndexApplyGeneric_H

// EOF

