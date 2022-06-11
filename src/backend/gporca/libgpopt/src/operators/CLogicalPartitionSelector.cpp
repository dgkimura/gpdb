//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalPartitionSelector.cpp
//
//	@doc:
//		Implementation of Logical partition selector
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalPartitionSelector.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::CLogicalPartitionSelector
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalPartitionSelector::CLogicalPartitionSelector(CMemoryPool *mp)
	: CLogical(mp),
	  m_mdid(nullptr),
	  m_pdrgpexprFilters(nullptr),
	  m_pcrOid(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::CLogicalPartitionSelector
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalPartitionSelector::CLogicalPartitionSelector(
	CMemoryPool *mp, gpos::Ref<IMDId> mdid,
	gpos::Ref<CExpressionArray> pdrgpexprFilters, CColRef *pcrOid)
	: CLogical(mp),
	  m_mdid(std::move(mdid)),
	  m_pdrgpexprFilters(std::move(pdrgpexprFilters)),
	  m_pcrOid(pcrOid)
{
	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(nullptr != m_pdrgpexprFilters);
	GPOS_ASSERT(0 < m_pdrgpexprFilters->Size());
	GPOS_ASSERT(nullptr != pcrOid);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::~CLogicalPartitionSelector
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalPartitionSelector::~CLogicalPartitionSelector()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::Matches
//
//	@doc:
//		Match operators
//
//---------------------------------------------------------------------------
BOOL
CLogicalPartitionSelector::Matches(COperator *pop) const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	CLogicalPartitionSelector *popPartSelector =
		gpos::dyn_cast<CLogicalPartitionSelector>(pop);

	return popPartSelector->PcrOid() == m_pcrOid &&
		   popPartSelector->MDId()->Equals(m_mdid.get()) &&
		   popPartSelector->m_pdrgpexprFilters->Equals(
			   m_pdrgpexprFilters.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::HashValue
//
//	@doc:
//		Hash operator
//
//---------------------------------------------------------------------------
ULONG
CLogicalPartitionSelector::HashValue() const
{
	return gpos::CombineHashes(Eopid(), m_mdid->HashValue());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalPartitionSelector::PopCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	CColRef *pcrOid = CUtils::PcrRemap(m_pcrOid, colref_mapping, must_exist);
	gpos::Ref<CExpressionArray> pdrgpexpr =
		CUtils::PdrgpexprRemap(mp, m_pdrgpexprFilters.get(), colref_mapping);

	;

	return GPOS_NEW(mp)
		CLogicalPartitionSelector(mp, m_mdid, std::move(pdrgpexpr), pcrOid);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalPartitionSelector::DeriveOutputColumns(CMemoryPool *mp,
											   CExpressionHandle &exprhdl)
{
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);

	pcrsOutput->Union(exprhdl.DeriveOutputColumns(0));
	pcrsOutput->Include(m_pcrOid);

	return pcrsOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalPartitionSelector::DeriveMaxCard(CMemoryPool *,	 // mp
										 CExpressionHandle &exprhdl) const
{
	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalPartitionSelector::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementPartitionSelector);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalPartitionSelector::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalPartitionSelector::OsPrint(IOstream &os) const
{
	os << SzId() << ", Part Table: ";
	m_mdid->OsPrint(os);

	return os;
}

// EOF
