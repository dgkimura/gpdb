//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CXform.cpp
//
//	@doc:
//		Base class for all transformations
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXform.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"


using namespace gpopt;

FORCE_GENERATE_DBGSTR(CXform);

//---------------------------------------------------------------------------
//	@function:
//		CXform::CXform
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CXform::CXform(gpos::Ref<CExpression> pexpr) : m_pexpr(std::move(pexpr))
{
	GPOS_ASSERT(nullptr != m_pexpr);
	GPOS_ASSERT(FCheckPattern(m_pexpr.get()));
}


//---------------------------------------------------------------------------
//	@function:
//		CXform::~CXform
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CXform::~CXform()
{
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CXform::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CXform::OsPrint(IOstream &os) const
{
	os << "Xform: " << SzId();

	if (GPOS_FTRACE(EopttracePrintXformPattern))
	{
		os << std::endl << "Pattern:" << std::endl << *m_pexpr;
	}

	return os;
}


#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CXform::FCheckPattern
//
//	@doc:
//		check a given expression against the pattern
//
//---------------------------------------------------------------------------
BOOL
CXform::FCheckPattern(gpos::Ref<CExpression> pexpr) const
{
	return pexpr->FMatchPattern(PexprPattern());
}


//---------------------------------------------------------------------------
//	@function:
//		CXform::FPromising
//
//	@doc:
//		Verify xform promise for the given expression
//
//---------------------------------------------------------------------------
BOOL
CXform::FPromising(CMemoryPool *mp, const gpos::Ref<CXform> pxform,
				   gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pxform);
	GPOS_ASSERT(nullptr != pexpr);

	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexpr);
	exprhdl.DeriveProps(nullptr /*pdpctxt*/);

	return ExfpNone < pxform->Exfp(exprhdl);
}

#endif	// GPOS_DEBUG


//---------------------------------------------------------------------------
//	@function:
//		CXform::FEqualIds
//
//	@doc:
//		Equality function on xform ids
//
//---------------------------------------------------------------------------
BOOL
CXform::FEqualIds(const CHAR *szIdOne, const CHAR *szIdTwo)
{
	return 0 == clib::Strcmp(szIdOne, szIdTwo);
}

//---------------------------------------------------------------------------
//	@function:
//		CXform::PbsNLJoinXforms
//
//	@doc:
//		Returns a set containing all xforms related to nestloop join.
//		Caller takes ownership of the returned set
//
//---------------------------------------------------------------------------
gpos::Ref<CBitSet>
CXform::PbsNLJoinXforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfInnerJoin2NLJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftOuterJoin2NLJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftSemiJoin2NLJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftAntiSemiJoin2NLJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftAntiSemiJoinNotIn2NLJoinNotIn));

	return pbs;
}

//---------------------------------------------------------------------------
//	@function:
//		CXform::PbsIndexJoinXforms
//
//	@doc:
//		Returns a set containing all xforms related to index join.
//		Caller takes ownership of the returned set
//
//---------------------------------------------------------------------------
gpos::Ref<CBitSet>
CXform::PbsIndexJoinXforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfJoin2BitmapIndexGetApply));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfJoin2IndexGetApply));

	return pbs;
}

//---------------------------------------------------------------------------
//	@function:
//		CXform::PbsBitmapIndexXforms
//
//	@doc:
//		Returns a set containing all xforms related to bitmap indexes.
//		Caller takes ownership of the returned set
//
//---------------------------------------------------------------------------
gpos::Ref<CBitSet>
CXform::PbsBitmapIndexXforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfSelect2BitmapBoolOp));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfSelect2DynamicBitmapBoolOp));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfJoin2BitmapIndexGetApply));

	return pbs;
}

//	returns a set containing all xforms that generate a plan with hash join
//	Caller takes ownership of the returned set
gpos::Ref<CBitSet>
CXform::PbsHashJoinXforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);

	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfInnerJoin2HashJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftOuterJoin2HashJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftSemiJoin2HashJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftAntiSemiJoin2HashJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfLeftAntiSemiJoinNotIn2HashJoinNotIn));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfRightOuterJoin2HashJoin));
	(void) pbs->ExchangeSet(GPOPT_DISABLE_XFORM_TF(
		CXform::
			ExfLeftJoin2RightJoin));  // Right joins are only used with hash joins, so disable this too
	return pbs;
}

gpos::Ref<CBitSet>
CXform::PbsJoinOrderInQueryXforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);

	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinDP));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinDPv2));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinMinCard));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfJoinAssociativity));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfJoinCommutativity));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinGreedy));

	return pbs;
}

gpos::Ref<CBitSet>
CXform::PbsJoinOrderOnGreedyXforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);

	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinDP));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinDPv2));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfJoinAssociativity));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfJoinCommutativity));

	return pbs;
}

gpos::Ref<CBitSet>
CXform::PbsJoinOrderOnExhaustiveXforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);

	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinDPv2));

	return pbs;
}

gpos::Ref<CBitSet>
CXform::PbsJoinOrderOnExhaustive2Xforms(CMemoryPool *mp)
{
	gpos::Ref<CBitSet> pbs = GPOS_NEW(mp) CBitSet(mp, EopttraceSentinel);

	(void) pbs->ExchangeSet(GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoin));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinDP));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinMinCard));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfExpandNAryJoinGreedy));
	(void) pbs->ExchangeSet(
		GPOPT_DISABLE_XFORM_TF(CXform::ExfPushDownLeftOuterJoin));
	(void) pbs->ExchangeSet(EopttraceEnableLOJInNAryJoin);

	return pbs;
}

BOOL
CXform::IsApplyOnce()
{
	return false;
}
// EOF
