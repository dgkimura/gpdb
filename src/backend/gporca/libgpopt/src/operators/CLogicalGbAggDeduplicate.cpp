//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalGbAggDeduplicate.cpp
//
//	@doc:
//		Implementation of aggregate operator for deduplicating semijoin outputs
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalGbAggDeduplicate.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
//
//	@doc:
//		Ctor for xform pattern
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate(CMemoryPool *mp)
	: CLogicalGbAgg(mp), m_pdrgpcrKeys(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate(
	CMemoryPool *mp, gpos::owner<CColRefArray *> colref_array,
	COperator::EGbAggType egbaggtype, gpos::owner<CColRefArray *> pdrgpcrKeys)
	: CLogicalGbAgg(mp, colref_array, egbaggtype),
	  m_pdrgpcrKeys(std::move(pdrgpcrKeys))
{
	GPOS_ASSERT(nullptr != m_pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate(
	CMemoryPool *mp, gpos::owner<CColRefArray *> colref_array,
	gpos::owner<CColRefArray *> pdrgpcrMinimal,
	COperator::EGbAggType egbaggtype, gpos::owner<CColRefArray *> pdrgpcrKeys)
	: CLogicalGbAgg(mp, colref_array, pdrgpcrMinimal, egbaggtype),
	  m_pdrgpcrKeys(std::move(pdrgpcrKeys))
{
	GPOS_ASSERT(nullptr != m_pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::~CLogicalGbAggDeduplicate
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::~CLogicalGbAggDeduplicate()
{
	// safe release -- to allow for instances used in patterns
	CRefCount::SafeRelease(m_pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalGbAggDeduplicate::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> colref_array =
		CUtils::PdrgpcrRemap(mp, Pdrgpcr(), colref_mapping, must_exist);

	gpos::owner<CColRefArray *> pdrgpcrMinimal = PdrgpcrMinimal();
	if (nullptr != pdrgpcrMinimal)
	{
		pdrgpcrMinimal = CUtils::PdrgpcrRemap(mp, pdrgpcrMinimal,
											  colref_mapping, must_exist);
	}

	gpos::owner<CColRefArray *> pdrgpcrKeys =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrKeys, colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalGbAggDeduplicate(
		mp, std::move(colref_array), std::move(pdrgpcrMinimal), Egbaggtype(),
		std::move(pdrgpcrKeys));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PcrsStat
//
//	@doc:
//		Compute required stats columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalGbAggDeduplicate::PcrsStat(CMemoryPool *mp, CExpressionHandle &exprhdl,
								   gpos::pointer<CColRefSet *> pcrsInput,
								   ULONG child_index) const
{
	return PcrsStatGbAgg(mp, exprhdl, pcrsInput, child_index, m_pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalGbAggDeduplicate::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   CUtils::UlHashColArray(Pdrgpcr()));
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrKeys));

	ULONG ulGbaggtype = (ULONG) Egbaggtype();

	ulHash = gpos::CombineHashes(ulHash, gpos::HashValue<ULONG>(&ulGbaggtype));

	return gpos::CombineHashes(ulHash,
							   gpos::HashValue<BOOL>(&m_fGeneratesDuplicates));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogicalGbAggDeduplicate::DeriveKeyCollection(CMemoryPool *mp,
											  CExpressionHandle &  //exprhdl
) const
{
	gpos::owner<CKeyCollection *> pkc = nullptr;

	// Gb produces a key only if it's global
	if (FGlobal())
	{
		// keys from join child are still keys
		m_pdrgpcrKeys->AddRef();
		pkc = GPOS_NEW(mp) CKeyCollection(mp, m_pdrgpcrKeys);
	}

	return pkc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalGbAggDeduplicate::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	gpos::pointer<CLogicalGbAggDeduplicate *> popAgg =
		gpos::dyn_cast<CLogicalGbAggDeduplicate>(pop);

	if (FGeneratesDuplicates() != popAgg->FGeneratesDuplicates())
	{
		return false;
	}

	return popAgg->Egbaggtype() == Egbaggtype() &&
		   Pdrgpcr()->Equals(popAgg->Pdrgpcr()) &&
		   m_pdrgpcrKeys->Equals(popAgg->PdrgpcrKeys()) &&
		   CColRef::Equals(PdrgpcrMinimal(), popAgg->PdrgpcrMinimal());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalGbAggDeduplicate::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfPushGbDedupBelowJoin);
	(void) xform_set->ExchangeSet(CXform::ExfSplitGbAggDedup);
	(void) xform_set->ExchangeSet(CXform::ExfGbAggDedup2HashAggDedup);
	(void) xform_set->ExchangeSet(CXform::ExfGbAggDedup2StreamAggDedup);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalGbAggDeduplicate::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<IStatisticsArray *>  // not used
) const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);
	gpos::pointer<IStatistics *> child_stats = exprhdl.Pstats(0);

	// extract computed columns
	gpos::owner<ULongPtrArray *> pdrgpulComputedCols =
		GPOS_NEW(mp) ULongPtrArray(mp);
	exprhdl.DeriveDefinedColumns(1)->ExtractColIds(mp, pdrgpulComputedCols);

	// construct bitset with keys of join child
	gpos::owner<CBitSet *> keys = GPOS_NEW(mp) CBitSet(mp);
	const ULONG ulKeys = m_pdrgpcrKeys->Size();
	for (ULONG ul = 0; ul < ulKeys; ul++)
	{
		CColRef *colref = (*m_pdrgpcrKeys)[ul];
		keys->ExchangeSet(colref->Id());
	}

	IStatistics *stats = CLogicalGbAgg::PstatsDerive(mp, child_stats, Pdrgpcr(),
													 pdrgpulComputedCols, keys);
	keys->Release();
	pdrgpulComputedCols->Release();

	return stats;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalGbAggDeduplicate::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << "( ";
	OsPrintGbAggType(os, Egbaggtype());
	os << " )";
	os << " Grp Cols: [";
	CUtils::OsPrintDrgPcr(os, Pdrgpcr());
	os << "], Minimal Grp Cols: [";
	if (nullptr != PdrgpcrMinimal())
	{
		CUtils::OsPrintDrgPcr(os, PdrgpcrMinimal());
	}
	os << "], Join Child Keys: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrKeys);
	os << "]";
	os << ", Generates Duplicates :[ " << FGeneratesDuplicates() << " ] ";

	return os;
}

// EOF
