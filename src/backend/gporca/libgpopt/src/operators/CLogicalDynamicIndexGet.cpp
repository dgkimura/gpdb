//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDynamicIndexGet.cpp
//
//	@doc:
//		Implementation of index access for partitioned tables
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDynamicIndexGet.h"

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::CLogicalDynamicIndexGet
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDynamicIndexGet::CLogicalDynamicIndexGet(CMemoryPool *mp)
	: CLogicalDynamicGetBase(mp),
	  m_pindexdesc(nullptr),
	  m_ulOriginOpId(gpos::ulong_max),
	  m_pos(nullptr)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::CLogicalDynamicIndexGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicIndexGet::CLogicalDynamicIndexGet(
	CMemoryPool *mp, const IMDIndex *pmdindex,
	gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId,
	const CName *pnameAlias, ULONG part_idx_id,
	gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart,
	gpos::Ref<IMdIdArray> partition_mdids)
	: CLogicalDynamicGetBase(
		  mp, pnameAlias, ptabdesc, part_idx_id, std::move(pdrgpcrOutput),
		  std::move(pdrgpdrgpcrPart), std::move(partition_mdids)),
	  m_pindexdesc(nullptr),
	  m_ulOriginOpId(ulOriginOpId)
{
	GPOS_ASSERT(nullptr != pmdindex);

	// create the index descriptor
	m_pindexdesc = CIndexDescriptor::Pindexdesc(mp, ptabdesc.get(), pmdindex);

	// compute the order spec
	m_pos = PosFromIndex(m_mp, pmdindex, m_pdrgpcrOutput.get(), ptabdesc.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::~CLogicalDynamicIndexGet
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDynamicIndexGet::~CLogicalDynamicIndexGet()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDynamicIndexGet::HashValue() const
{
	return gpos::CombineHashes(
		COperator::HashValue(),
		gpos::CombineHashes(gpos::HashValue(&m_scan_id),
							m_pindexdesc->MDId()->HashValue()));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicIndexGet::Matches(COperator *pop) const
{
	return CUtils::FMatchDynamicIndex(this, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::DeriveOuterReferences
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalDynamicIndexGet::DeriveOuterReferences(CMemoryPool *mp,
											   CExpressionHandle &exprhdl)
{
	return PcrsDeriveOuterIndexGet(mp, exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalDynamicIndexGet::PopCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDIndex *pmdindex = md_accessor->RetrieveIndex(m_pindexdesc->MDId());
	CName *pnameAlias = GPOS_NEW(mp) CName(mp, *m_pnameAlias);

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

	gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrPart.get(), colref_mapping, must_exist);

	;
	;

	return GPOS_NEW(mp)
		CLogicalDynamicIndexGet(mp, pmdindex, m_ptabdesc, m_ulOriginOpId,
								pnameAlias, m_scan_id, std::move(pdrgpcrOutput),
								std::move(pdrgpdrgpcrPart), m_partition_mdids);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::FInputOrderSensitive
//
//	@doc:
//		Is input order sensitive
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicIndexGet::FInputOrderSensitive() const
{
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalDynamicIndexGet::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfDynamicIndexGet2DynamicIndexScan);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::PstatsDerive
//
//	@doc:
//		Load up statistics from metadata
//
//---------------------------------------------------------------------------

gpos::Ref<IStatistics>
CLogicalDynamicIndexGet::PstatsDerive(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  IStatisticsArray *stats_ctxt) const
{
	return CStatisticsUtils::DeriveStatsForIndexGet(mp, exprhdl, stats_ctxt);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDynamicIndexGet::OsPrint(IOstream &os) const
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
	os << "), ";
	os << "Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput.get());
	os << "] Scan Id: " << m_scan_id;


	return os;
}

// EOF
