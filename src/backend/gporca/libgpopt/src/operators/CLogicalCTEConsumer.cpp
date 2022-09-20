//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalCTEConsumer.cpp
//
//	@doc:
//		Implementation of CTE consumer operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalCTEConsumer.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CMaxCard.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalCTEProducer.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::CLogicalCTEConsumer
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalCTEConsumer::CLogicalCTEConsumer(CMemoryPool *mp)
	: CLogical(mp),
	  m_id(0),
	  m_pdrgpcr(nullptr),
	  m_pexprInlined(nullptr),
	  m_phmulcr(nullptr),
	  m_pcrsOutput(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::CLogicalCTEConsumer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalCTEConsumer::CLogicalCTEConsumer(
	CMemoryPool *mp, ULONG id, gpos::owner<CColRefArray *> colref_array)
	: CLogical(mp),
	  m_id(id),
	  m_pdrgpcr(std::move(colref_array)),
	  m_pexprInlined(nullptr),
	  m_phmulcr(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdrgpcr);
	m_pcrsOutput = GPOS_NEW(mp) CColRefSet(mp, m_pdrgpcr);
	CreateInlinedExpr(mp);
	m_pcrsLocalUsed->Include(m_pdrgpcr);

	// map consumer columns to their positions in consumer output
	COptCtxt::PoctxtFromTLS()->Pcteinfo()->AddConsumerCols(id, m_pdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::~CLogicalCTEConsumer
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalCTEConsumer::~CLogicalCTEConsumer()
{
	CRefCount::SafeRelease(m_pdrgpcr);
	CRefCount::SafeRelease(m_pexprInlined);
	CRefCount::SafeRelease(m_phmulcr);
	CRefCount::SafeRelease(m_pcrsOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::CreateInlinedExpr
//
//	@doc:
//		Create the inlined version of this consumer as well as the column mapping
//
//---------------------------------------------------------------------------
void
CLogicalCTEConsumer::CreateInlinedExpr(CMemoryPool *mp)
{
	gpos::pointer<CExpression *> pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);
	// the actual definition of the CTE is the first child of the producer
	gpos::pointer<CExpression *> pexprCTEDef = (*pexprProducer)[0];

	gpos::pointer<CLogicalCTEProducer *> popProducer =
		gpos::dyn_cast<CLogicalCTEProducer>(pexprProducer->Pop());

	m_phmulcr = CUtils::PhmulcrMapping(mp, popProducer->Pdrgpcr(), m_pdrgpcr);
	m_pexprInlined = pexprCTEDef->PexprCopyWithRemappedColumns(
		mp, m_phmulcr, true /*must_exist*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalCTEConsumer::DeriveOutputColumns(CMemoryPool *,		  //mp,
										 CExpressionHandle &  //exprhdl
)
{
	m_pcrsOutput->AddRef();
	return m_pcrsOutput;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::DeriveNotNullColumns
//
//	@doc:
//		Derive not nullable output columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalCTEConsumer::DeriveNotNullColumns(CMemoryPool *mp,
										  CExpressionHandle &  // exprhdl
) const
{
	gpos::pointer<CExpression *> pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);

	// find producer's not null columns
	gpos::pointer<CColRefSet *> pcrsProducerNotNull =
		pexprProducer->DeriveNotNullColumns();

	// map producer's not null columns to consumer's output columns
	gpos::owner<CColRefSet *> pcrsConsumerNotNull = CUtils::PcrsRemap(
		mp, pcrsProducerNotNull, m_phmulcr, true /*must_exist*/);
	GPOS_ASSERT(pcrsConsumerNotNull->Size() == pcrsProducerNotNull->Size());

	return pcrsConsumerNotNull;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogicalCTEConsumer::DeriveKeyCollection(CMemoryPool *,		  //mp,
										 CExpressionHandle &  //exprhdl
) const
{
	gpos::pointer<CExpression *> pexpr =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexpr);
	CKeyCollection *pkc = pexpr->DeriveKeyCollection();
	if (nullptr != pkc)
	{
		pkc->AddRef();
	}

	return pkc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::DerivePartitionInfo
//
//	@doc:
//		Derive partition consumers
//
//---------------------------------------------------------------------------
gpos::owner<CPartInfo *>
CLogicalCTEConsumer::DerivePartitionInfo(CMemoryPool *,		  //mp,
										 CExpressionHandle &  //exprhdl
) const
{
	gpos::owner<CPartInfo *> ppartInfo = m_pexprInlined->DerivePartitionInfo();
	ppartInfo->AddRef();

	return ppartInfo;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalCTEConsumer::DeriveMaxCard(CMemoryPool *,		//mp,
								   CExpressionHandle &	//exprhdl
) const
{
	gpos::pointer<CExpression *> pexpr =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexpr);
	return pexpr->DeriveMaxCard();
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::DeriveJoinDepth
//
//	@doc:
//		Derive join depth
//
//---------------------------------------------------------------------------
ULONG
CLogicalCTEConsumer::DeriveJoinDepth(CMemoryPool *,		  //mp,
									 CExpressionHandle &  //exprhdl
) const
{
	gpos::pointer<CExpression *> pexpr =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexpr);
	return pexpr->DeriveJoinDepth();
}

// derive table descriptor
gpos::pointer<CTableDescriptor *>
CLogicalCTEConsumer::DeriveTableDescriptor(CMemoryPool *,		//mp
										   CExpressionHandle &	//exprhdl
) const
{
	gpos::pointer<CExpression *> pexpr =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexpr);
	return pexpr->DeriveTableDescriptor();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalCTEConsumer::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	gpos::pointer<CLogicalCTEConsumer *> popCTEConsumer =
		gpos::dyn_cast<CLogicalCTEConsumer>(pop);

	return m_id == popCTEConsumer->UlCTEId() &&
		   m_pdrgpcr->Equals(popCTEConsumer->Pdrgpcr());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalCTEConsumer::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), m_id);
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcr));

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::FInputOrderSensitive
//
//	@doc:
//		Not called for leaf operators
//
//---------------------------------------------------------------------------
BOOL
CLogicalCTEConsumer::FInputOrderSensitive() const
{
	GPOS_ASSERT(!"Unexpected function call of FInputOrderSensitive");
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::owner<COperator *>
CLogicalCTEConsumer::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CColRefArray *> colref_array = nullptr;
	if (must_exist)
	{
		colref_array =
			CUtils::PdrgpcrRemapAndCreate(mp, m_pdrgpcr, colref_mapping);
	}
	else
	{
		colref_array =
			CUtils::PdrgpcrRemap(mp, m_pdrgpcr, colref_mapping, must_exist);
	}
	return GPOS_NEW(mp) CLogicalCTEConsumer(mp, m_id, std::move(colref_array));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalCTEConsumer::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfInlineCTEConsumer);
	(void) xform_set->ExchangeSet(CXform::ExfImplementCTEConsumer);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::DerivePropertyConstraint
//
//	@doc:
//		Derive constraint property
//
//---------------------------------------------------------------------------
gpos::owner<CPropConstraint *>
CLogicalCTEConsumer::DerivePropertyConstraint(CMemoryPool *mp,
											  CExpressionHandle &  //exprhdl
) const
{
	gpos::pointer<CExpression *> pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);
	gpos::pointer<CPropConstraint *> ppc =
		pexprProducer->DerivePropertyConstraint();
	gpos::pointer<CColRefSetArray *> pdrgpcrs = ppc->PdrgpcrsEquivClasses();
	gpos::pointer<CConstraint *> pcnstr = ppc->Pcnstr();

	// remap producer columns to consumer columns
	gpos::owner<CColRefSetArray *> pdrgpcrsMapped =
		GPOS_NEW(mp) CColRefSetArray(mp);
	const ULONG length = pdrgpcrs->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CColRefSet *> pcrs = (*pdrgpcrs)[ul];
		gpos::owner<CColRefSet *> pcrsMapped =
			CUtils::PcrsRemap(mp, pcrs, m_phmulcr, true /*must_exist*/);
		pdrgpcrsMapped->Append(pcrsMapped);
	}

	gpos::owner<CConstraint *> pcnstrMapped = nullptr;
	if (nullptr != pcnstr)
	{
		pcnstrMapped = pcnstr->PcnstrCopyWithRemappedColumns(
			mp, m_phmulcr, true /*must_exist*/);
	}

	return GPOS_NEW(mp)
		CPropConstraint(mp, std::move(pdrgpcrsMapped), std::move(pcnstrMapped));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::PstatsDerive
//
//	@doc:
//		Derive statistics based on cte producer
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogicalCTEConsumer::PstatsDerive(
	CMemoryPool *mp,
	CExpressionHandle &,			   //exprhdl,
	gpos::pointer<IStatisticsArray *>  // statistics_array
) const
{
	gpos::pointer<CExpression *> pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);
	gpos::pointer<const IStatistics *> stats = pexprProducer->Pstats();
	GPOS_ASSERT(nullptr != stats);

	// copy the stats with the remaped colids
	IStatistics *new_stats = stats->CopyStatsWithRemap(mp, m_phmulcr);

	return new_stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEConsumer::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalCTEConsumer::OsPrint(IOstream &os) const
{
	os << SzId() << " (";
	os << m_id;
	os << "), Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcr);
	os << "]";

	return os;
}

// EOF
