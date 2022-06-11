//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalDynamicBitmapTableGet.cpp
//
//	@doc:
//		Logical operator for dynamic table access via bitmap indexes.
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDynamicBitmapTableGet.h"

#include "gpos/common/owner.h"

#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/xforms/CXform.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet
//
//	@doc:
//		Ctor
//		Takes ownership of ptabdesc, pnameTableAlias and pdrgpcrOutput.
//
//---------------------------------------------------------------------------
CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet(
	CMemoryPool *mp, gpos::owner<CTableDescriptor *> ptabdesc,
	ULONG ulOriginOpId, const CName *pnameTableAlias, ULONG ulPartIndex,
	gpos::owner<CColRefArray *> pdrgpcrOutput,
	gpos::owner<CColRef2dArray *> pdrgpdrgpcrPart,
	gpos::owner<IMdIdArray *> partition_mdids)
	: CLogicalDynamicGetBase(mp, pnameTableAlias, std::move(ptabdesc),
							 ulPartIndex, std::move(pdrgpcrOutput),
							 std::move(pdrgpdrgpcrPart),
							 std::move(partition_mdids)),
	  m_ulOriginOpId(ulOriginOpId)

{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet(CMemoryPool *mp)
	: CLogicalDynamicGetBase(mp), m_ulOriginOpId(gpos::ulong_max)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::~CLogicalDynamicBitmapTableGet
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDynamicBitmapTableGet::~CLogicalDynamicBitmapTableGet() = default;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDynamicBitmapTableGet::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash, gpos::HashValue(&m_scan_id));
	ulHash =
		gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOutput));

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::Matches
//
//	@doc:
//		Match this operator with the given one.
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicBitmapTableGet::Matches(gpos::pointer<COperator *> pop) const
{
	return CUtils::FMatchDynamicBitmapScan(this, pop);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::DerivePropertyConstraint
//
//	@doc:
//		Derive the constraint property.
//
//---------------------------------------------------------------------------
gpos::owner<CPropConstraint *>
CLogicalDynamicBitmapTableGet::DerivePropertyConstraint(
	CMemoryPool *mp, CExpressionHandle &exprhdl) const
{
	return PpcDeriveConstraintFromTableWithPredicates(mp, exprhdl, m_ptabdesc,
													  m_pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::DeriveOuterReferences
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalDynamicBitmapTableGet::DeriveOuterReferences(CMemoryPool *mp,
													 CExpressionHandle &exprhdl)
{
	return PcrsDeriveOuterIndexGet(mp, exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogicalDynamicBitmapTableGet::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<IStatisticsArray *> stats_ctxt) const
{
	return CStatisticsUtils::DeriveStatsForBitmapTableGet(mp, exprhdl,
														  stats_ctxt);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::OsPrint
//
//	@doc:
//		Debug print of this operator
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDynamicBitmapTableGet::OsPrint(IOstream &os) const
{
	os << SzId() << " ";
	os << ", Table Name: (";
	m_ptabdesc->Name().OsPrint(os);
	os << ") Scan Id: " << m_scan_id;
	os << ", Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput);
	os << "]";

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalDynamicBitmapTableGet::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrOutput = nullptr;
	if (must_exist)
	{
		pdrgpcrOutput =
			CUtils::PdrgpcrRemapAndCreate(mp, m_pdrgpcrOutput, colref_mapping);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(mp, m_pdrgpcrOutput,
											 colref_mapping, must_exist);
	}
	CName *pnameAlias = GPOS_NEW(mp) CName(mp, *m_pnameAlias);

	m_ptabdesc->AddRef();
	m_partition_mdids->AddRef();

	gpos::owner<CColRef2dArray *> pdrgpdrgpcrPart = CUtils::PdrgpdrgpcrRemap(
		mp, m_pdrgpdrgpcrPart, colref_mapping, must_exist);

	return GPOS_NEW(mp) CLogicalDynamicBitmapTableGet(
		mp, m_ptabdesc, m_ulOriginOpId, pnameAlias, m_scan_id,
		std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrPart),
		m_partition_mdids);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalDynamicBitmapTableGet::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementDynamicBitmapTableGet);

	return xform_set;
}

// EOF
