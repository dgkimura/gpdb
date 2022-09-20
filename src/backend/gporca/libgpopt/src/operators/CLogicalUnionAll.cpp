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
CLogicalUnionAll::CLogicalUnionAll(CMemoryPool *mp,
								   gpos::Ref<CColRefArray> pdrgpcrOutput,
								   gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput)
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
gpos::Ref<COperator>
CLogicalUnionAll::PopCopyWithRemappedColumns(CMemoryPool *mp,
											 UlongToColRefMap *colref_mapping,
											 BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrOutput = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrOutput.get(), colref_mapping, must_exist);
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrInput.get(), colref_mapping, must_exist);

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
gpos::Ref<CKeyCollection>
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
gpos::Ref<CXformSet>
CLogicalUnionAll::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
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
gpos::Ref<IStatistics>
CLogicalUnionAll::PstatsDeriveUnionAll(CMemoryPool *mp,
									   CExpressionHandle &exprhdl)
{
	GPOS_ASSERT(COperator::EopLogicalUnionAll == exprhdl.Pop()->Eopid() ||
				COperator::EopLogicalUnion == exprhdl.Pop()->Eopid());

	CColRefArray *pdrgpcrOutput =
		gpos::dyn_cast<CLogicalSetOp>(exprhdl.Pop())->PdrgpcrOutput();
	CColRef2dArray *pdrgpdrgpcrInput =
		gpos::dyn_cast<CLogicalSetOp>(exprhdl.Pop())->PdrgpdrgpcrInput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);
	GPOS_ASSERT(nullptr != pdrgpdrgpcrInput);

	gpos::Ref<IStatistics> result_stats = exprhdl.Pstats(0);
	;
	const ULONG arity = exprhdl.Arity();
	for (ULONG ul = 1; ul < arity; ul++)
	{
		IStatistics *child_stats = exprhdl.Pstats(ul);
		gpos::Ref<CStatistics> stats =
			CUnionAllStatsProcessor::CreateStatsForUnionAll(
				mp, gpos::dyn_cast<CStatistics>(result_stats.get()),
				dynamic_cast<CStatistics *>(child_stats),
				CColRef::Pdrgpul(mp, pdrgpcrOutput),
				CColRef::Pdrgpul(mp, (*pdrgpdrgpcrInput)[0].get()),
				CColRef::Pdrgpul(mp, (*pdrgpdrgpcrInput)[ul].get()));
		;
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
gpos::Ref<IStatistics>
CLogicalUnionAll::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
							   IStatisticsArray *  // not used
) const
{
	GPOS_ASSERT(EspNone < Esp(exprhdl));

	return PstatsDeriveUnionAll(mp, exprhdl);
}

// EOF
