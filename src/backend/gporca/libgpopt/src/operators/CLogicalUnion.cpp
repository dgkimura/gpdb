//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogicalUnion.cpp
//
//	@doc:
//		Implementation of union operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalUnion.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalUnionAll.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnion::CLogicalUnion
//
//	@doc:
//		ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalUnion::CLogicalUnion(CMemoryPool *mp) : CLogicalSetOp(mp)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnion::CLogicalUnion
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalUnion::CLogicalUnion(CMemoryPool *mp,
							 gpos::Ref<CColRefArray> pdrgpcrOutput,
							 gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput)
	: CLogicalSetOp(mp, std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrInput))
{
#ifdef GPOS_DEBUG
	CColRefArray *pdrgpcrInput = (*pdrgpdrgpcrInput)[0].get();
	const ULONG num_cols = pdrgpcrOutput->Size();
	GPOS_ASSERT(num_cols == pdrgpcrInput->Size());

	// Ensure that the output columns are the same as first input
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		GPOS_ASSERT((*pdrgpcrOutput)[ul] == (*pdrgpcrInput)[ul]);
	}

#endif	// GPOS_DEBUG
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnion::~CLogicalUnion
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalUnion::~CLogicalUnion() = default;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnion::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalUnion::PopCopyWithRemappedColumns(CMemoryPool *mp,
										  UlongToColRefMap *colref_mapping,
										  BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrOutput = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrOutput.get(), colref_mapping, must_exist);
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrInput.get(), colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalUnion(mp, std::move(pdrgpcrOutput),
									  std::move(pdrgpdrgpcrInput));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnion::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalUnion::DeriveMaxCard(CMemoryPool *,	 // mp
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
//		CLogicalUnion::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalUnion::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfUnion2UnionAll);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnion::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalUnion::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
							IStatisticsArray *	// not used
) const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	// union is transformed into a group by over an union all
	// we follow the same route to compute statistics
	gpos::Ref<IStatistics> pstatsUnionAll =
		CLogicalUnionAll::PstatsDeriveUnionAll(mp, exprhdl);

	// computed columns
	gpos::Ref<ULongPtrArray> pdrgpulComputedCols =
		GPOS_NEW(mp) ULongPtrArray(mp);

	gpos::Ref<IStatistics> stats = CLogicalGbAgg::PstatsDerive(
		mp, pstatsUnionAll.get(),
		m_pdrgpcrOutput.get(),		// we group by the output columns
		pdrgpulComputedCols.get(),	// no computed columns for set ops
		nullptr						// no keys, use all grouping cols
	);

	// clean up
	;
	;

	return stats;
}


// EOF
