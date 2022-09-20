//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalIndexGet.cpp
//
//	@doc:
//		Implementation of basic index access
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalIndexGet.h"

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::CLogicalIndexGet
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalIndexGet::CLogicalIndexGet(CMemoryPool *mp)
	: CLogical(mp),
	  m_pindexdesc(nullptr),
	  m_ptabdesc(nullptr),
	  m_ulOriginOpId(gpos::ulong_max),
	  m_pnameAlias(nullptr),
	  m_pdrgpcrOutput(nullptr),
	  m_pcrsOutput(nullptr),
	  m_pos(nullptr),
	  m_pcrsDist(nullptr)
{
	m_fPattern = true;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::CLogicalIndexGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalIndexGet::CLogicalIndexGet(CMemoryPool *mp, const IMDIndex *pmdindex,
								   gpos::Ref<CTableDescriptor> ptabdesc,
								   ULONG ulOriginOpId, const CName *pnameAlias,
								   gpos::Ref<CColRefArray> pdrgpcrOutput)
	: CLogical(mp),
	  m_pindexdesc(nullptr),
	  m_ptabdesc(std::move(ptabdesc)),
	  m_ulOriginOpId(ulOriginOpId),
	  m_pnameAlias(pnameAlias),
	  m_pdrgpcrOutput(std::move(pdrgpcrOutput)),
	  m_pcrsOutput(nullptr),
	  m_pcrsDist(nullptr)
{
	GPOS_ASSERT(nullptr != pmdindex);
	GPOS_ASSERT(nullptr != m_ptabdesc);
	GPOS_ASSERT(nullptr != pnameAlias);
	GPOS_ASSERT(nullptr != m_pdrgpcrOutput);

	// create the index descriptor
	m_pindexdesc = CIndexDescriptor::Pindexdesc(mp, m_ptabdesc.get(), pmdindex);

	// compute the order spec
	m_pos =
		PosFromIndex(m_mp, pmdindex, m_pdrgpcrOutput.get(), m_ptabdesc.get());

	// create a set representation of output columns
	m_pcrsOutput = GPOS_NEW(mp) CColRefSet(mp, m_pdrgpcrOutput.get());

	m_pcrsDist =
		CLogical::PcrsDist(mp, m_ptabdesc.get(), m_pdrgpcrOutput.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::~CLogicalIndexGet
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalIndexGet::~CLogicalIndexGet()
{
	;
	;
	;
	;
	;
	;

	GPOS_DELETE(m_pnameAlias);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalIndexGet::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   m_pindexdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash,
								 CUtils::UlHashColArray(m_pdrgpcrOutput.get()));
	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalIndexGet::Matches(COperator *pop) const
{
	return CUtils::FMatchIndex(this, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalIndexGet::PopCopyWithRemappedColumns(CMemoryPool *mp,
											 UlongToColRefMap *colref_mapping,
											 BOOL must_exist)
{
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDIndex *pmdindex = md_accessor->RetrieveIndex(m_pindexdesc->MDId());

	gpos::Ref<CColRefArray> pdrgpcrOutput = nullptr;
	if (must_exist)
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemapAndCreate(mp, m_pdrgpcrOutput.get(),
													  colref_mapping);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(mp, m_pdrgpcrOutput.get(),
											 colref_mapping, must_exist);
	}
	CName *pnameAlias = GPOS_NEW(mp) CName(mp, *m_pnameAlias);

	;

	return GPOS_NEW(mp)
		CLogicalIndexGet(mp, pmdindex, m_ptabdesc, m_ulOriginOpId, pnameAlias,
						 std::move(pdrgpcrOutput));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalIndexGet::DeriveOutputColumns(CMemoryPool *mp,
									  CExpressionHandle &  // exprhdl
)
{
	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(m_pdrgpcrOutput.get());

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::DeriveOuterReferences
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalIndexGet::DeriveOuterReferences(CMemoryPool *mp,
										CExpressionHandle &exprhdl)
{
	return PcrsDeriveOuterIndexGet(mp, exprhdl);
}

gpos::Ref<CKeyCollection>
CLogicalIndexGet::DeriveKeyCollection(CMemoryPool *mp,
									  CExpressionHandle &  // exprhdl
) const
{
	const CBitSetArray *pdrgpbs = m_ptabdesc->PdrgpbsKeys();

	return CLogical::PkcKeysBaseTable(mp, pdrgpbs, m_pdrgpcrOutput.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::FInputOrderSensitive
//
//	@doc:
//		Is input order sensitive
//
//---------------------------------------------------------------------------
BOOL
CLogicalIndexGet::FInputOrderSensitive() const
{
	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalIndexGet::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);

	(void) xform_set->ExchangeSet(CXform::ExfIndexGet2IndexScan);
	(void) xform_set->ExchangeSet(CXform::ExfIndexGet2IndexOnlyScan);

	return xform_set;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalIndexGet::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
							   IStatisticsArray *stats_ctxt) const
{
	return CStatisticsUtils::DeriveStatsForIndexGet(mp, exprhdl, stats_ctxt);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalIndexGet::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalIndexGet::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " ";
	// index name
	os << "  Index Name: (";
	m_pindexdesc->Name().OsPrint(os);
	// table alias name
	os << ")";
	os << ", Table Name: (";
	m_pnameAlias->OsPrint(os);
	os << ")";
	os << ", Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput.get());
	os << "]";

	return os;
}

// EOF
