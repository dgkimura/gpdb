//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDelete.cpp
//
//	@doc:
//		Implementation of logical Delete operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDelete.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::CLogicalDelete
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDelete::CLogicalDelete(CMemoryPool *mp)
	: CLogical(mp),
	  m_ptabdesc(nullptr),
	  m_pdrgpcr(nullptr),
	  m_pcrCtid(nullptr),
	  m_pcrSegmentId(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::CLogicalDelete
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDelete::CLogicalDelete(CMemoryPool *mp,
							   gpos::owner<CTableDescriptor *> ptabdesc,
							   gpos::owner<CColRefArray *> colref_array,
							   CColRef *pcrCtid, CColRef *pcrSegmentId)
	: CLogical(mp),
	  m_ptabdesc(std::move(ptabdesc)),
	  m_pdrgpcr(std::move(colref_array)),
	  m_pcrCtid(pcrCtid),
	  m_pcrSegmentId(pcrSegmentId)

{
	GPOS_ASSERT(nullptr != m_ptabdesc);
	GPOS_ASSERT(nullptr != m_pdrgpcr);
	GPOS_ASSERT(nullptr != pcrCtid);
	GPOS_ASSERT(nullptr != pcrSegmentId);

	m_pcrsLocalUsed->Include(m_pdrgpcr);
	m_pcrsLocalUsed->Include(m_pcrCtid);
	m_pcrsLocalUsed->Include(m_pcrSegmentId);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::~CLogicalDelete
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDelete::~CLogicalDelete()
{
	CRefCount::SafeRelease(m_ptabdesc);
	CRefCount::SafeRelease(m_pdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalDelete::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	gpos::pointer<CLogicalDelete *> popDelete =
		gpos::dyn_cast<CLogicalDelete>(pop);

	return m_pcrCtid == popDelete->PcrCtid() &&
		   m_pcrSegmentId == popDelete->PcrSegmentId() &&
		   m_ptabdesc->MDId()->Equals(popDelete->Ptabdesc()->MDId()) &&
		   m_pdrgpcr->Equals(popDelete->Pdrgpcr());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDelete::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcr));
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrCtid));
	ulHash =
		gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrSegmentId));

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalDelete::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> colref_array =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcr, colref_mapping, must_exist);
	CColRef *pcrCtid = CUtils::PcrRemap(m_pcrCtid, colref_mapping, must_exist);
	CColRef *pcrSegmentId =
		CUtils::PcrRemap(m_pcrSegmentId, colref_mapping, must_exist);
	m_ptabdesc->AddRef();

	return GPOS_NEW(mp) CLogicalDelete(mp, m_ptabdesc, std::move(colref_array),
									   pcrCtid, pcrSegmentId);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalDelete::DeriveOutputColumns(CMemoryPool *mp,
									CExpressionHandle &	 //exprhdl
)
{
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);
	pcrsOutput->Include(m_pdrgpcr);
	return pcrsOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogicalDelete::DeriveKeyCollection(CMemoryPool *,	// mp
									CExpressionHandle &exprhdl) const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalDelete::DeriveMaxCard(CMemoryPool *,  // mp
							  CExpressionHandle &exprhdl) const
{
	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalDelete::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfDelete2DML);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogicalDelete::PstatsDerive(CMemoryPool *,	 // mp,
							 CExpressionHandle &exprhdl,
							 gpos::pointer<IStatisticsArray *>	// not used
) const
{
	return PstatsPassThruOuter(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDelete::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDelete::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " (";
	m_ptabdesc->Name().OsPrint(os);
	os << "), Deleted Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcr);
	os << "], ";
	m_pcrCtid->OsPrint(os);
	os << ", ";
	m_pcrSegmentId->OsPrint(os);

	return os;
}

// EOF
