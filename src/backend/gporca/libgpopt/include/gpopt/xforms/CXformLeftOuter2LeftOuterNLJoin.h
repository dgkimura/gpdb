//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CXformLeftOuter2LeftOuterNLJoin.h
//
//	@doc:
//		Simplify Left Outer Join with constant false predicate
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformLeftOuter2LeftOuterNLJoin_H
#define GPOPT_CXformLeftOuter2LeftOuterNLJoin_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CXformLeftOuter2LeftOuterNLJoin
	//
	//	@doc:
	//		Simplify Left Outer Join with constant false predicate
	//
	//---------------------------------------------------------------------------
	class CXformLeftOuter2LeftOuterNLJoin : public CXformExploration
	{

		private:

			// private copy ctor
			CXformLeftOuter2LeftOuterNLJoin(const CXformLeftOuter2LeftOuterNLJoin &);

		public:

			// ctor
			explicit
			CXformLeftOuter2LeftOuterNLJoin(CMemoryPool *mp);

			// dtor
			virtual
			~CXformLeftOuter2LeftOuterNLJoin()
			{}

			// ident accessors
			virtual
			EXformId Exfid() const
			{
				return ExfLeftOuter2LeftOuterNLJoin;
			}

			// return a string for xform name
			virtual
			const CHAR *SzId() const
			{
				return "CXformLeftOuter2LeftOuterNLJoin";
			}

			// Compatibility function for simplifying aggregates
			virtual
			BOOL FCompatible
				(
				CXform::EXformId exfid
				)
			{
				return (CXform::ExfLeftOuter2LeftOuterNLJoin != exfid);
			}

			// compute xform promise for a given expression handle
			virtual
			EXformPromise Exfp (CExpressionHandle &exprhdl) const;

			// actual transform
			virtual
			void Transform(CXformContext *pxfctxt, CXformResult *pxfres, CExpression *pexpr) const;

	}; // class CXformLeftOuter2LeftOuterNLJoin

}

#endif // !GPOPT_CXformLeftOuter2LeftOuterNLJoin_H

// EOF
