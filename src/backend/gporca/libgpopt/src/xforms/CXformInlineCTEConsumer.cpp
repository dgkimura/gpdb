//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformInlineCTEConsumer.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformInlineCTEConsumer.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformInlineCTEConsumer::CXformInlineCTEConsumer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformInlineCTEConsumer::CXformInlineCTEConsumer(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalCTEConsumer(mp)))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformInlineCTEConsumer::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformInlineCTEConsumer::Exfp(CExpressionHandle &exprhdl) const
{
	const ULONG id =
		gpos::dyn_cast<CLogicalCTEConsumer>(exprhdl.Pop())->UlCTEId();
	CCTEInfo *pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();

	if ((pcteinfo->FEnableInlining() || 1 == pcteinfo->UlConsumers(id)) &&
		CXformUtils::FInlinableCTE(id))
	{
		return CXform::ExfpHigh;
	}

	return CXform::ExfpNone;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformInlineCTEConsumer::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformInlineCTEConsumer::Transform(CXformContext *
#ifdef GPOS_DEBUG
									   pxfctxt
#endif
								   ,
								   CXformResult *pxfres,
								   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	// inline the consumer
	CLogicalCTEConsumer *popConsumer =
		gpos::dyn_cast<CLogicalCTEConsumer>(pexpr->Pop());
	gpos::Ref<CExpression> pexprAlt = popConsumer->PexprInlined();
	;
	// add alternative to xform result
	pxfres->Add(std::move(pexprAlt));
}

// EOF
