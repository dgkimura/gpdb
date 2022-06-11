//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalSplit.cpp
//
//	@doc:
//		Implementation of logical split operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalSplit.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::CLogicalSplit
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalSplit::CLogicalSplit(CMemoryPool *mp)
	: CLogical(mp),
	  m_pdrgpcrDelete(nullptr),
	  m_pdrgpcrInsert(nullptr),
	  m_pcrCtid(nullptr),
	  m_pcrSegmentId(nullptr),
	  m_pcrAction(nullptr),
	  m_pcrTupleOid(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::CLogicalSplit
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalSplit::CLogicalSplit(CMemoryPool *mp,
							 gpos::owner<CColRefArray *> pdrgpcrDelete,
							 gpos::owner<CColRefArray *> pdrgpcrInsert,
							 CColRef *pcrCtid, CColRef *pcrSegmentId,
							 CColRef *pcrAction, CColRef *pcrTupleOid)
	: CLogical(mp),
	  m_pdrgpcrDelete(std::move(pdrgpcrDelete)),
	  m_pdrgpcrInsert(std::move(pdrgpcrInsert)),
	  m_pcrCtid(pcrCtid),
	  m_pcrSegmentId(pcrSegmentId),
	  m_pcrAction(pcrAction),
	  m_pcrTupleOid(pcrTupleOid)

{
	GPOS_ASSERT(nullptr != m_pdrgpcrDelete);
	GPOS_ASSERT(nullptr != m_pdrgpcrInsert);
	GPOS_ASSERT(m_pdrgpcrInsert->Size() == m_pdrgpcrDelete->Size());
	GPOS_ASSERT(nullptr != pcrCtid);
	GPOS_ASSERT(nullptr != pcrSegmentId);
	GPOS_ASSERT(nullptr != pcrAction);

	m_pcrsLocalUsed->Include(m_pdrgpcrDelete);
	m_pcrsLocalUsed->Include(m_pdrgpcrInsert);
	m_pcrsLocalUsed->Include(m_pcrCtid);
	m_pcrsLocalUsed->Include(m_pcrSegmentId);
	m_pcrsLocalUsed->Include(m_pcrAction);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::~CLogicalSplit
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalSplit::~CLogicalSplit()
{
	CRefCount::SafeRelease(m_pdrgpcrDelete);
	CRefCount::SafeRelease(m_pdrgpcrInsert);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalSplit::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() == Eopid())
	{
		gpos::pointer<CLogicalSplit *> popSplit =
			gpos::dyn_cast<CLogicalSplit>(pop);

		return m_pcrCtid == popSplit->PcrCtid() &&
			   m_pcrSegmentId == popSplit->PcrSegmentId() &&
			   m_pcrAction == popSplit->PcrAction() &&
			   m_pcrTupleOid == popSplit->PcrTupleOid() &&
			   m_pdrgpcrDelete->Equals(popSplit->PdrgpcrDelete()) &&
			   m_pdrgpcrInsert->Equals(popSplit->PdrgpcrInsert());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalSplit::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   CUtils::UlHashColArray(m_pdrgpcrInsert));
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrCtid));
	ulHash =
		gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrSegmentId));
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrAction));

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalSplit::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> pdrgpcrDelete =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrDelete, colref_mapping, must_exist);
	gpos::owner<CColRefArray *> pdrgpcrInsert =
		CUtils::PdrgpcrRemap(mp, m_pdrgpcrInsert, colref_mapping, must_exist);
	CColRef *pcrCtid = CUtils::PcrRemap(m_pcrCtid, colref_mapping, must_exist);
	CColRef *pcrSegmentId =
		CUtils::PcrRemap(m_pcrSegmentId, colref_mapping, must_exist);
	CColRef *pcrAction =
		CUtils::PcrRemap(m_pcrAction, colref_mapping, must_exist);

	CColRef *pcrTupleOid = nullptr;
	if (nullptr != m_pcrTupleOid)
	{
		pcrTupleOid =
			CUtils::PcrRemap(m_pcrTupleOid, colref_mapping, must_exist);
	}

	return GPOS_NEW(mp)
		CLogicalSplit(mp, std::move(pdrgpcrDelete), std::move(pdrgpcrInsert),
					  pcrCtid, pcrSegmentId, pcrAction, pcrTupleOid);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalSplit::DeriveOutputColumns(CMemoryPool *mp, CExpressionHandle &exprhdl)
{
	GPOS_ASSERT(2 == exprhdl.Arity());

	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Union(exprhdl.DeriveOutputColumns(0));
	pcrs->Include(m_pcrAction);

	if (nullptr != m_pcrTupleOid)
	{
		pcrs->Include(m_pcrTupleOid);
	}

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogicalSplit::DeriveKeyCollection(CMemoryPool *,  // mp
								   CExpressionHandle &exprhdl) const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalSplit::DeriveMaxCard(CMemoryPool *,	 // mp
							 CExpressionHandle &exprhdl) const
{
	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalSplit::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementSplit);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogicalSplit::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
							gpos::pointer<IStatisticsArray *>  // not used
) const
{
	// split returns double the number of tuples coming from its child
	gpos::pointer<IStatistics *> stats = exprhdl.Pstats(0);

	return stats->ScaleStats(mp, CDouble(2.0) /*factor*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalSplit::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " -- Delete Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrDelete);
	os << "], Insert Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrInsert);
	os << "], ";
	m_pcrCtid->OsPrint(os);
	os << ", ";
	m_pcrSegmentId->OsPrint(os);
	os << ", ";
	m_pcrAction->OsPrint(os);

	return os;
}

// EOF
