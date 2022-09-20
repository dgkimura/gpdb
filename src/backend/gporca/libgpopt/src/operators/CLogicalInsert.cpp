//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalInsert.cpp
//
//	@doc:
//		Implementation of logical Insert operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalInsert.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::CLogicalInsert
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalInsert::CLogicalInsert(CMemoryPool *mp)
	: CLogical(mp), m_ptabdesc(nullptr), m_pdrgpcrSource(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::CLogicalInsert
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalInsert::CLogicalInsert(CMemoryPool *mp,
							   gpos::Ref<CTableDescriptor> ptabdesc,
							   gpos::Ref<CColRefArray> pdrgpcrSource)
	: CLogical(mp),
	  m_ptabdesc(std::move(ptabdesc)),
	  m_pdrgpcrSource(std::move(pdrgpcrSource))

{
	GPOS_ASSERT(nullptr != m_ptabdesc);
	GPOS_ASSERT(nullptr != m_pdrgpcrSource);

	m_pcrsLocalUsed->Include(m_pdrgpcrSource.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::~CLogicalInsert
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalInsert::~CLogicalInsert()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalInsert::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalInsert *popInsert = gpos::dyn_cast<CLogicalInsert>(pop);

	return m_ptabdesc->MDId()->Equals(popInsert->Ptabdesc()->MDId()) &&
		   m_pdrgpcrSource->Equals(popInsert->PdrgpcrSource());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalInsert::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash,
								 CUtils::UlHashColArray(m_pdrgpcrSource.get()));

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalInsert::PopCopyWithRemappedColumns(CMemoryPool *mp,
										   UlongToColRefMap *colref_mapping,
										   BOOL must_exist)
{
	gpos::Ref<CColRefArray> colref_array = CUtils::PdrgpcrRemap(
		mp, m_pdrgpcrSource.get(), colref_mapping, must_exist);
	;

	return GPOS_NEW(mp) CLogicalInsert(mp, m_ptabdesc, std::move(colref_array));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalInsert::DeriveOutputColumns(CMemoryPool *mp,
									CExpressionHandle &	 //exprhdl
)
{
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);
	pcrsOutput->Include(m_pdrgpcrSource.get());
	return pcrsOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::Ref<CKeyCollection>
CLogicalInsert::DeriveKeyCollection(CMemoryPool *,	// mp
									CExpressionHandle &exprhdl) const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalInsert::DeriveMaxCard(CMemoryPool *,  // mp
							  CExpressionHandle &exprhdl) const
{
	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalInsert::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfInsert2DML);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalInsert::PstatsDerive(CMemoryPool *,	 // mp,
							 CExpressionHandle &exprhdl,
							 IStatisticsArray *	 // not used
) const
{
	return PstatsPassThruOuter(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalInsert::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalInsert::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " (";
	m_ptabdesc->Name().OsPrint(os);
	os << "), Source Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrSource.get());
	os << "]";

	return os;
}

// EOF
