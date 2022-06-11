//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformUnnestTVF.h
//
//	@doc:
//		 Unnest TVF with subquery arguments
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformUnnestTVF_H
#define GPOPT_CXformUnnestTVF_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformUnnestTVF
//
//	@doc:
//		Unnest TVF with subquery arguments
//
//---------------------------------------------------------------------------
class CXformUnnestTVF : public CXformExploration
{
private:
	// helper for mapping subquery function arguments into columns
	static gpos::owner<CColRefArray *> PdrgpcrSubqueries(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprCTEProducer,
		gpos::pointer<CExpression *> pexprCTEConsumer);

	//	collect subquery arguments and return a Project expression
	static gpos::owner<CExpression *> PexprProjectSubqueries(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprTVF);

public:
	CXformUnnestTVF(const CXformUnnestTVF &) = delete;

	// ctor
	explicit CXformUnnestTVF(CMemoryPool *mp);

	// dtor
	~CXformUnnestTVF() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfUnnestTVF;
	}

	// return a string for xform name
	const CHAR *
	SzId() const override
	{
		return "CXformUnnestTVF";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(gpos::pointer<CXformContext *> pxfctxt,
				   gpos::pointer<CXformResult *> pxfres,
				   gpos::pointer<CExpression *> pexpr) const override;

};	// class CXformUnnestTVF

}  // namespace gpopt

#endif	// !GPOPT_CXformUnnestTVF_H

// EOF
