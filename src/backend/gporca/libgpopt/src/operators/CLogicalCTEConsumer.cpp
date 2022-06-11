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
CLogicalCTEConsumer::CLogicalCTEConsumer(CMemoryPool *mp, ULONG id,
										 gpos::Ref<CColRefArray> colref_array)
	: CLogical(mp),
	  m_id(id),
	  m_pdrgpcr(std::move(colref_array)),
	  m_pexprInlined(nullptr),
	  m_phmulcr(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdrgpcr);
	m_pcrsOutput = GPOS_NEW(mp) CColRefSet(mp, m_pdrgpcr.get());
	CreateInlinedExpr(mp);
	m_pcrsLocalUsed->Include(m_pdrgpcr.get());

	// map consumer columns to their positions in consumer output
	COptCtxt::PoctxtFromTLS()->Pcteinfo()->AddConsumerCols(id, m_pdrgpcr.get());
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
	;
	;
	;
	;
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
	CExpression *pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);
	// the actual definition of the CTE is the first child of the producer
	CExpression *pexprCTEDef = (*pexprProducer)[0];

	CLogicalCTEProducer *popProducer =
		gpos::dyn_cast<CLogicalCTEProducer>(pexprProducer->Pop());

	m_phmulcr =
		CUtils::PhmulcrMapping(mp, popProducer->Pdrgpcr(), m_pdrgpcr.get());
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
gpos::Ref<CColRefSet>
CLogicalCTEConsumer::DeriveOutputColumns(CMemoryPool *,		  //mp,
										 CExpressionHandle &  //exprhdl
)
{
	;
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
gpos::Ref<CColRefSet>
CLogicalCTEConsumer::DeriveNotNullColumns(CMemoryPool *mp,
										  CExpressionHandle &  // exprhdl
) const
{
	CExpression *pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);

	// find producer's not null columns
	CColRefSet *pcrsProducerNotNull = pexprProducer->DeriveNotNullColumns();

	// map producer's not null columns to consumer's output columns
	gpos::Ref<CColRefSet> pcrsConsumerNotNull = CUtils::PcrsRemap(
		mp, pcrsProducerNotNull, m_phmulcr.get(), true /*must_exist*/);
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
gpos::Ref<CKeyCollection>
CLogicalCTEConsumer::DeriveKeyCollection(CMemoryPool *,		  //mp,
										 CExpressionHandle &  //exprhdl
) const
{
	CExpression *pexpr =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexpr);
	CKeyCollection *pkc = pexpr->DeriveKeyCollection();
	if (nullptr != pkc)
	{
		;
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
gpos::Ref<CPartInfo>
CLogicalCTEConsumer::DerivePartitionInfo(CMemoryPool *,		  //mp,
										 CExpressionHandle &  //exprhdl
) const
{
	gpos::Ref<CPartInfo> ppartInfo = m_pexprInlined->DerivePartitionInfo();
	;

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
	CExpression *pexpr =
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
	CExpression *pexpr =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexpr);
	return pexpr->DeriveJoinDepth();
}

// derive table descriptor
CTableDescriptor *
CLogicalCTEConsumer::DeriveTableDescriptor(CMemoryPool *,		//mp
										   CExpressionHandle &	//exprhdl
) const
{
	CExpression *pexpr =
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
CLogicalCTEConsumer::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalCTEConsumer *popCTEConsumer =
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
	ulHash =
		gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcr.get()));

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
gpos::Ref<COperator>
CLogicalCTEConsumer::PopCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	gpos::Ref<CColRefArray> colref_array = nullptr;
	if (must_exist)
	{
		colref_array =
			CUtils::PdrgpcrRemapAndCreate(mp, m_pdrgpcr.get(), colref_mapping);
	}
	else
	{
		colref_array = CUtils::PdrgpcrRemap(mp, m_pdrgpcr.get(), colref_mapping,
											must_exist);
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
gpos::Ref<CXformSet>
CLogicalCTEConsumer::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
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
gpos::Ref<CPropConstraint>
CLogicalCTEConsumer::DerivePropertyConstraint(CMemoryPool *mp,
											  CExpressionHandle &  //exprhdl
) const
{
	CExpression *pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);
	CPropConstraint *ppc = pexprProducer->DerivePropertyConstraint();
	CColRefSetArray *pdrgpcrs = ppc->PdrgpcrsEquivClasses();
	CConstraint *pcnstr = ppc->Pcnstr();

	// remap producer columns to consumer columns
	gpos::Ref<CColRefSetArray> pdrgpcrsMapped =
		GPOS_NEW(mp) CColRefSetArray(mp);
	const ULONG length = pdrgpcrs->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CColRefSet *pcrs = (*pdrgpcrs)[ul].get();
		gpos::Ref<CColRefSet> pcrsMapped =
			CUtils::PcrsRemap(mp, pcrs, m_phmulcr.get(), true /*must_exist*/);
		pdrgpcrsMapped->Append(pcrsMapped);
	}

	gpos::Ref<CConstraint> pcnstrMapped = nullptr;
	if (nullptr != pcnstr)
	{
		pcnstrMapped = pcnstr->PcnstrCopyWithRemappedColumns(
			mp, m_phmulcr.get(), true /*must_exist*/);
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
gpos::Ref<IStatistics>
CLogicalCTEConsumer::PstatsDerive(CMemoryPool *mp,
								  CExpressionHandle &,	//exprhdl,
								  IStatisticsArray *	// statistics_array
) const
{
	CExpression *pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);
	const IStatistics *stats = pexprProducer->Pstats();
	GPOS_ASSERT(nullptr != stats);

	// copy the stats with the remaped colids
	gpos::Ref<IStatistics> new_stats =
		stats->CopyStatsWithRemap(mp, m_phmulcr.get());

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
	CUtils::OsPrintDrgPcr(os, m_pdrgpcr.get());
	os << "]";

	return os;
}

// EOF
