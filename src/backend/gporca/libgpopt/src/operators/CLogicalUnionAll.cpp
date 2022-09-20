//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalUnionAll.cpp
//
//	@doc:
//		Implementation of UnionAll operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalUnionAll.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/statistics/CUnionAllStatsProcessor.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::CLogicalUnionAll
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalUnionAll::CLogicalUnionAll(CMemoryPool *mp) : CLogicalUnion(mp)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::CLogicalUnionAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalUnionAll::CLogicalUnionAll(
	CMemoryPool *mp, gpos::owner<CColRefArray *> pdrgpcrOutput,
	gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput)
	: CLogicalUnion(mp, std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrInput))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::~CLogicalUnionAll
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalUnionAll::~CLogicalUnionAll() = default;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalUnionAll::DeriveMaxCard(CMemoryPool *,	// mp
								CExpressionHandle &exprhdl) const
{
	const ULONG arity = exprhdl.Arity();

	CMaxCard maxcard = exprhdl.DeriveMaxCard(0);
	for (ULONG ul = 1; ul < arity; ul++)
	{
		maxcard += exprhdl.DeriveMaxCard(ul);
	}

	return maxcard;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalUnionAll::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrOutput =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrOutput, colref_mapping, must_exist);
	gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrInput, colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalUnionAll(mp, std::move(pdrgpcrOutput),
										 std::move(pdrgpdrgpcrInput));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogicalUnionAll::DeriveKeyCollection(CMemoryPool *,	   //mp,
									  CExpressionHandle &  // exprhdl
) const
{
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalUnionAll::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementUnionAll);

	return xform_set;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PstatsDeriveUnionAll
//
//	@doc:
//		Derive statistics based on union all semantics
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogicalUnionAll::PstatsDeriveUnionAll(CMemoryPool *mp,
									   CExpressionHandle &exprhdl)
{
	GPOS_ASSERT(COperator::EopLogicalUnionAll == exprhdl.Pop()->Eopid() ||
				COperator::EopLogicalUnion == exprhdl.Pop()->Eopid());

	gpos::pointer<CColRefArray *> pdrgpcrOutput =
		gpos::dyn_cast<CLogicalSetOp>(exprhdl.Pop())->PdrgpcrOutput();
	gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput =
		gpos::dyn_cast<CLogicalSetOp>(exprhdl.Pop())->PdrgpdrgpcrInput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);
	GPOS_ASSERT(nullptr != pdrgpdrgpcrInput);

	gpos::owner<IStatistics *> result_stats = exprhdl.Pstats(0);
	result_stats->AddRef();
	const ULONG arity = exprhdl.Arity();
	for (ULONG ul = 1; ul < arity; ul++)
	{
		gpos::pointer<IStatistics *> child_stats = exprhdl.Pstats(ul);
		CStatistics *stats = CUnionAllStatsProcessor::CreateStatsForUnionAll(
			mp, gpos::dyn_cast<CStatistics>(result_stats),
			dynamic_cast<CStatistics *>(child_stats),
			CColRef::Pdrgpul(mp, pdrgpcrOutput),
			CColRef::Pdrgpul(mp, (*pdrgpdrgpcrInput)[0]),
			CColRef::Pdrgpul(mp, (*pdrgpdrgpcrInput)[ul]));
		result_stats->Release();
		result_stats = stats;
	}

	return result_stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PstatsDerive
//
//	@doc:
//		Derive statistics based on union all semantics
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogicalUnionAll::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
							   gpos::pointer<IStatisticsArray *>  // not used
) const
{
	GPOS_ASSERT(EspNone < Esp(exprhdl));

	return PstatsDeriveUnionAll(mp, exprhdl);
}

// EOF
