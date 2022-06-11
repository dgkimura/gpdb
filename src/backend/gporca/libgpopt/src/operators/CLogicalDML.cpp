//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDML.cpp
//
//	@doc:
//		Implementation of logical DML operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDML.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"


using namespace gpopt;

const WCHAR CLogicalDML::m_rgwszDml[EdmlSentinel][10] = {
	GPOS_WSZ_LIT("Insert"), GPOS_WSZ_LIT("Delete"), GPOS_WSZ_LIT("Update")};

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::CLogicalDML
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDML::CLogicalDML(CMemoryPool *mp)
	: CLogical(mp),
	  m_ptabdesc(nullptr),
	  m_pdrgpcrSource(nullptr),
	  m_pbsModified(nullptr),
	  m_pcrAction(nullptr),
	  m_pcrTableOid(nullptr),
	  m_pcrCtid(nullptr),
	  m_pcrSegmentId(nullptr),
	  m_pcrTupleOid(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::CLogicalDML
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDML::CLogicalDML(CMemoryPool *mp, EDMLOperator edmlop,
						 gpos::Ref<CTableDescriptor> ptabdesc,
						 gpos::Ref<CColRefArray> pdrgpcrSource,
						 gpos::Ref<CBitSet> pbsModified, CColRef *pcrAction,
						 CColRef *pcrTableOid, CColRef *pcrCtid,
						 CColRef *pcrSegmentId, CColRef *pcrTupleOid)
	: CLogical(mp),
	  m_edmlop(edmlop),
	  m_ptabdesc(ptabdesc),
	  m_pdrgpcrSource(pdrgpcrSource),
	  m_pbsModified(pbsModified),
	  m_pcrAction(pcrAction),
	  m_pcrTableOid(pcrTableOid),
	  m_pcrCtid(pcrCtid),
	  m_pcrSegmentId(pcrSegmentId),
	  m_pcrTupleOid(pcrTupleOid)
{
	GPOS_ASSERT(EdmlSentinel != edmlop);
	GPOS_ASSERT(nullptr != m_ptabdesc);
	GPOS_ASSERT(nullptr != m_pdrgpcrSource);
	GPOS_ASSERT(nullptr != m_pbsModified);
	GPOS_ASSERT(nullptr != pcrAction);
	GPOS_ASSERT_IMP(EdmlDelete == edmlop || EdmlUpdate == edmlop,
					nullptr != pcrCtid && nullptr != pcrSegmentId);

	m_pcrsLocalUsed->Include(m_pdrgpcrSource.get());
	m_pcrsLocalUsed->Include(m_pcrAction);
	if (nullptr != m_pcrTableOid)
	{
		m_pcrsLocalUsed->Include(m_pcrTableOid);
	}
	if (nullptr != m_pcrCtid)
	{
		m_pcrsLocalUsed->Include(m_pcrCtid);
	}

	if (nullptr != m_pcrSegmentId)
	{
		m_pcrsLocalUsed->Include(m_pcrSegmentId);
	}

	if (nullptr != m_pcrTupleOid)
	{
		m_pcrsLocalUsed->Include(m_pcrTupleOid);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::~CLogicalDML
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDML::~CLogicalDML()
{
	;
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalDML::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalDML *popDML = gpos::dyn_cast<CLogicalDML>(pop);

	return m_pcrAction == popDML->PcrAction() &&
		   m_pcrTableOid == popDML->PcrTableOid() &&
		   m_pcrCtid == popDML->PcrCtid() &&
		   m_pcrSegmentId == popDML->PcrSegmentId() &&
		   m_pcrTupleOid == popDML->PcrTupleOid() &&
		   m_ptabdesc->MDId()->Equals(popDML->Ptabdesc()->MDId()) &&
		   m_pdrgpcrSource->Equals(popDML->PdrgpcrSource());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDML::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrAction));
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrTableOid));
	ulHash = gpos::CombineHashes(ulHash,
								 CUtils::UlHashColArray(m_pdrgpcrSource.get()));

	if (EdmlDelete == m_edmlop || EdmlUpdate == m_edmlop)
	{
		ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrCtid));
		ulHash =
			gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrSegmentId));
	}

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalDML::PopCopyWithRemappedColumns(CMemoryPool *mp,
										UlongToColRefMap *colref_mapping,
										BOOL must_exist)
{
	gpos::Ref<CColRefArray> colref_array = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrSource.get(), colref_mapping, must_exist);
	CColRef *pcrAction =
		CUtils::PcrRemap(m_pcrAction, colref_mapping, must_exist);
	CColRef *pcrTableOid =
		CUtils::PcrRemap(m_pcrTableOid, colref_mapping, must_exist);

	// no need to remap modified columns bitset as it represent column indexes
	// and not actual columns
	;

	CColRef *pcrCtid = nullptr;
	if (nullptr != m_pcrCtid)
	{
		pcrCtid = CUtils::PcrRemap(m_pcrCtid, colref_mapping, must_exist);
	}

	CColRef *pcrSegmentId = nullptr;
	if (nullptr != m_pcrSegmentId)
	{
		pcrSegmentId =
			CUtils::PcrRemap(m_pcrSegmentId, colref_mapping, must_exist);
	}

	CColRef *pcrTupleOid = nullptr;
	if (nullptr != m_pcrTupleOid)
	{
		pcrTupleOid =
			CUtils::PcrRemap(m_pcrTupleOid, colref_mapping, must_exist);
	}

	;

	return GPOS_NEW(mp) CLogicalDML(
		mp, m_edmlop, m_ptabdesc, std::move(colref_array), m_pbsModified,
		pcrAction, pcrTableOid, pcrCtid, pcrSegmentId, pcrTupleOid);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalDML::DeriveOutputColumns(CMemoryPool *mp,
								 CExpressionHandle &  //exprhdl
)
{
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);
	pcrsOutput->Include(m_pdrgpcrSource.get());
	if (nullptr != m_pcrCtid)
	{
		GPOS_ASSERT(nullptr != m_pcrSegmentId);
		pcrsOutput->Include(m_pcrCtid);
		pcrsOutput->Include(m_pcrSegmentId);
	}

	pcrsOutput->Include(m_pcrAction);

	if (nullptr != m_pcrTupleOid)
	{
		pcrsOutput->Include(m_pcrTupleOid);
	}

	return pcrsOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::DerivePropertyConstraint
//
//	@doc:
//		Derive constraint property
//
//---------------------------------------------------------------------------
gpos::Ref<CPropConstraint>
CLogicalDML::DerivePropertyConstraint(CMemoryPool *mp,
									  CExpressionHandle &exprhdl) const
{
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);
	pcrsOutput->Include(m_pdrgpcrSource.get());
	gpos::Ref<CPropConstraint> ppc =
		PpcDeriveConstraintRestrict(mp, exprhdl, pcrsOutput.get());
	;

	return ppc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::Ref<CKeyCollection>
CLogicalDML::DeriveKeyCollection(CMemoryPool *,	 // mp
								 CExpressionHandle &exprhdl) const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalDML::DeriveMaxCard(CMemoryPool *,  // mp
						   CExpressionHandle &exprhdl) const
{
	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalDML::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementDML);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalDML::PstatsDerive(CMemoryPool *,  // mp,
						  CExpressionHandle &exprhdl,
						  IStatisticsArray *  // not used
) const
{
	return PstatsPassThruOuter(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDML::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDML::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " (";
	os << m_rgwszDml[m_edmlop] << ", ";
	m_ptabdesc->Name().OsPrint(os);
	os << "), Affected Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrSource.get());
	os << "], Action: (";
	m_pcrAction->OsPrint(os);
	os << ")";

	if (m_pcrTableOid != nullptr)
	{
		os << ", Oid: (";
		m_pcrTableOid->OsPrint(os);
		os << ")";
	}

	if (EdmlDelete == m_edmlop || EdmlUpdate == m_edmlop)
	{
		os << ", ";
		m_pcrCtid->OsPrint(os);
		os << ", ";
		m_pcrSegmentId->OsPrint(os);
	}

	return os;
}

// EOF
